/*******************************************************************************
**          File: client.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:05 PM
**   Description: see the header file 
*******************************************************************************/
#include "client.h"
#include <algorithm>
#include <cstring>
#include "nul/log.hpp"

namespace {
  #define SOCKS_ERROR_REPLY(replyField) "\5" replyField "\0\1\0\0\0\0\0\0"
  #define SOCKS_ERROR_REPLY_LENGTH 10
}

namespace sockspp {
  Client::Client(std::unique_ptr<uvcpp::Tcp> &&conn,
                 const std::shared_ptr<nul::BufferPool> &bufferPool) :
    downstreamConn_(std::move(conn)), bufferPool_(bufferPool) {
  }

  void Client::start() {
    downstreamConn_->once<uvcpp::EvClose>(
      // intentionally cycle-ref the Client object to avoid
      // deletion of it before this callback is fired
      [this, _ = shared_from_this()](const auto &e, auto &client){

      ipIt_ = ipAddrs_.end();
      if (upstreamConn_) {
        upstreamConn_->close();
      }
      if (dnsRequest_) {
        dnsRequest_->cancel();
      }
    });
    downstreamConn_->on<uvcpp::EvBufferRecycled>([this](const auto &e, auto &conn) {
      bufferPool_->returnBuffer(std::forward<std::unique_ptr<nul::Buffer>>(
          const_cast<uvcpp::EvBufferRecycled &>(e).buffer));
    });
    downstreamConn_->on<uvcpp::EvRead>(
      [this](const auto &e, auto &conn) {
      if (upstreamConnected_) {
        auto buffer = bufferPool_->assembleDataBuffer(e.buf, e.nread);
        upstreamConn_->writeAsync(std::move(buffer));
        return;
      }

      auto state = socks_.getState();
      if (state == Socks::State::ERROR) {
        // do nothing, shouldn't reach here

      } else {
        auto reply = socks_.parse(e.buf, e.nread);
        if (reply != Socks::ReplyField::SUCCEEDED) {
          this->replySocksError();
          conn.close();
          return;
        }

        if (state == Socks::State::METHOD_IDENTIFICATION)  {
          auto shouldUseUsernamePasswordAuth =
            !username_.empty() || !password_.empty();

          auto buffer = bufferPool_->requestBuffer(2);
          buffer->assign(shouldUseUsernamePasswordAuth ?  "\5\2" : "\5\0", 2);
          conn.writeAsync(std::move(buffer));

        } else if (state == Socks::State::USERNAME_PASSWORD)  {
          auto isCorrect = username_ == socks_.getParsedUsername() &&
            password_ == socks_.getParsedPassword();

          auto buffer = bufferPool_->requestBuffer(2);
          buffer->assign(isCorrect ? "\1\0" : "\1\1", 2);
          conn.writeAsync(std::move(buffer));

          if (!isCorrect) {
            LOG_E("username/password don't match");
            conn.close();

          } else {
            socks_.setState(Socks::State::PARSING_REQUEST);
          }

        } else if (state == Socks::State::PARSING_REQUEST)  {
          this->connectUpstream();

        }
      }
    });

    if (!username_.empty() || !password_.empty()) {
      socks_.setRequireAuthMethod(Socks::Method::USERNAME_PASSWORD);
    }

    downstreamConn_->readStart();
  }

  void Client::connectUpstream() {
    auto atyp = socks_.getAddressType();
    if (atyp == Socks::AddressType::IPV4) {
      uvcpp::SockAddr4 addr4;
      memcpy(&addr4.sin_addr, socks_.getAddress().data(), 4);
      addr4.sin_family = AF_INET;
      addr4.sin_port = socks_.getPort();

      connectUpstream(reinterpret_cast<uvcpp::SockAddr *>(&addr4));

    } else if (atyp == Socks::AddressType::IPV6) {
      uvcpp::SockAddr6 addr6;
      memcpy(&addr6.sin6_addr, socks_.getAddress().data(), 16);
      addr6.sin6_family = AF_INET6;
      addr6.sin6_port = socks_.getPort();

      connectUpstream(reinterpret_cast<uvcpp::SockAddr *>(&addr6));

    } else {
      dnsRequest_ = uvcpp::DNSRequest::createUnique(downstreamConn_->getLoop());
      dnsRequest_->once<uvcpp::EvDNSRequestFinish>(
        // intentionally cycle-ref the Client object to avoid
        // deletion of it before this callback is fired
        [this, _ = shared_from_this()](const auto &e, auto &req){
          dnsRequest_ = nullptr;
      });

      dnsRequest_->once<uvcpp::EvError>([this](const auto &e, auto &r) {
        LOG_W("Failed to resolve address: %s", socks_.getAddress().c_str());
        this->replySocksError();
        downstreamConn_->close();
      });

      dnsRequest_->once<uvcpp::EvDNSResult>(
        [this](const auto &e, auto &req) {
        if (e.dnsResults.empty()) {
          LOG_W("[%s] resolved to zero IPs", socks_.getAddress().c_str());
          this->replySocksError();
          downstreamConn_->close();
          return;
        }

        ipAddrs_ = std::move(e.dnsResults);
        ipIt_ = ipAddrs_.begin();
        auto newIp = *ipIt_;
        ++ipIt_;

        this->connectUpstream(newIp);
      });

      dnsRequest_->resolve(socks_.getAddress());
      LOG_D("Resolving address: %s", socks_.getAddress().c_str());
    }
  }

  void Client::connectUpstream(uvcpp::SockAddr *sockAddr) {
    createUpstreamConnection();
    if (!upstreamConn_->connect(sockAddr)) {
      upstreamConn_->close();
      replySocksError();
    }
  }

  void Client::connectUpstream(const std::string &ip) {
    createUpstreamConnection();
    if (!upstreamConn_->connect(ip, ntohs(socks_.getPort()))) {
      upstreamConn_->close();
      // check if there're more IPs to try
      if (ipIt_ == ipAddrs_.end()) {
        replySocksError();
      }
    }
  }

  void Client::createUpstreamConnection() {
    upstreamConn_ = uvcpp::Tcp::createShared(downstreamConn_->getLoop());
    upstreamConn_->once<uvcpp::EvError>(
      [this](const auto &e, auto &client) {
        if (!upstreamConnected_) {
          LOG_E("Failed to connect to: %s:%d",
                client.getIP().c_str(), client.getPort());
          this->replySocksError();
        }
      });
    upstreamConn_->once<uvcpp::EvClose>(
      // intentionally cycle-ref the Client object to avoid
      // deletion of it before this callback is fired
      [this, _ = shared_from_this()](const auto &e, auto &client){
      if (!upstreamConnected_ && ipIt_ != ipAddrs_.end()) {
        auto newIp = *ipIt_;
        ++ipIt_;
        this->connectUpstream(newIp);

      } else {
        downstreamConn_->close();
      }
    });
    upstreamConn_->once<uvcpp::EvConnect>(
      [this](const auto &e, auto &client) {
        upstreamConnected_ = true;
        LOG_V("Connected to: %s:%d", client.getIP().c_str(), client.getPort());

        auto sockAddr = upstreamConn_->getSockAddr();
        auto atyp = Socks::AddressType::UNKNOWN;
        auto bufLen = 4 + 2;  // first 4 bytes + length of port
        if (sockAddr->sa_family == AF_INET) {
          atyp = Socks::AddressType::IPV4;
          bufLen += 4;
        } else {
          atyp = Socks::AddressType::IPV6;
          bufLen += 16;
        }

        auto buffer = bufferPool_->requestBuffer(bufLen);
        auto data = buffer->getData();
        data[0] = '\5';
        data[1] = '\0';
        data[2] = '\0';

        if (sockAddr->sa_family == AF_INET) {
          data[3] = '\1';
          auto sockAddr4 = reinterpret_cast<const uvcpp::SockAddr4 *>(sockAddr);
          memcpy(data + 4, &sockAddr4->sin_addr, 4);
          memcpy(data + 8, &sockAddr4->sin_port, 2);
        } else {
          data[3] = '\4';
          auto sockAddr6 = reinterpret_cast<const uvcpp::SockAddr6 *>(sockAddr);
          memcpy(data + 4, &sockAddr6->sin6_addr, 16);
          memcpy(data + 20, &sockAddr6->sin6_port, 2);
        }
        buffer->setLength(bufLen);
        downstreamConn_->writeAsync(std::move(buffer));

        upstreamConn_->on<uvcpp::EvBufferRecycled>([this](const auto &e, auto &conn) {
          bufferPool_->returnBuffer(std::forward<std::unique_ptr<nul::Buffer>>(
              const_cast<uvcpp::EvBufferRecycled &>(e).buffer));
        });
        upstreamConn_->on<uvcpp::EvRead>([this](const auto &e, auto &client){
          auto buffer = bufferPool_->assembleDataBuffer(e.buf, e.nread);
          downstreamConn_->writeAsync(std::move(buffer));
        });

        upstreamConn_->readStart();
      });
  }

  void Client::replySocksError() {
    auto buffer = bufferPool_->requestBuffer(SOCKS_ERROR_REPLY_LENGTH);
    buffer->assign(SOCKS_ERROR_REPLY("\1"), SOCKS_ERROR_REPLY_LENGTH);
    downstreamConn_->writeAsync(std::move(buffer));
  }

  void Client::close() {
    downstreamConn_->close();
  }

  void Client::setUsername(const std::string &username) {
    username_ = username;
  }

  void Client::setPassword(const std::string &password) {
    password_ = password;
  }
} /* end of namspace: sockspp */
