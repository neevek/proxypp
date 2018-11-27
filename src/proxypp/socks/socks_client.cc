/*******************************************************************************
**          File: socks_client.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-20 Mon 05:02 PM
**   Description: see the header fiel 
*******************************************************************************/
#include "proxypp/socks/socks_client.h"
#include "nul/util.hpp"
#include <array>

namespace proxypp {

  SocksClient::SocksClient(
    const std::shared_ptr<uvcpp::Loop> &loop,
    std::shared_ptr<nul::BufferPool> bufferPool) :
    conn_(uvcpp::Tcp::createUnique(loop)),
    bufferPool_(bufferPool) {
  }

  bool SocksClient::connect(const std::string &serverHost, uint16_t serverPort) {
    return conn_->connect(serverHost, serverPort);
  }

  void SocksClient::startHandshake(
    const std::string &targetHost, uint16_t targetPort) {
    if (targetHost.empty() || targetPort == 0) {
      conn_->publish(EvSocksHandshake{false});
      return;
    }

    conn_->on<uvcpp::EvBufferRecycled>(
      [this](const auto &e, auto &conn) {
        bufferPool_->returnBuffer(std::forward<std::unique_ptr<nul::Buffer>>(
            const_cast<uvcpp::EvBufferRecycled &>(e).buffer));
      });

    conn_->on<uvcpp::EvRead>(
      [this, targetHost, targetPort](const auto &e, auto &client){

      auto state = socks_.getState();
      if (state == SocksRespParser::State::NEGOTIATION_COMPLETE) {
        conn_->publish(EvSocksRead{e.buf, e.nread});

      } else {
        if (!socks_.parse(e.buf, e.nread)) {
          client.close();
          conn_->publish(EvSocksHandshake{false});
          return;
        }

        state = socks_.getState();
        if (state == SocksRespParser::State::REQUEST) {
          this->sendSocksRequest(targetHost, targetPort);

        } else if (state == SocksRespParser::State::USERNAME_PASSWORD_AUTH) {
          auto authBuf = std::string{"\1"};
          authBuf.append(1, static_cast<char>(username_.length()));
          authBuf.append(username_);
          authBuf.append(1, static_cast<char>(password_.length()));
          authBuf.append(password_);
          client.writeAsync(
            bufferPool_->assembleDataBuffer(authBuf.c_str(), authBuf.length()));

        } else if (state == SocksRespParser::State::NEGOTIATION_COMPLETE) {
          conn_->publish(EvSocksHandshake{true});

        } else {
          // not possible to reach here
        }
      }
    });
    conn_->readStart();

    sendInitialRequest();
  }

  void SocksClient::sendInitialRequest() {
    int methods = 1;  // NO AUTHENTICATION REQUIRED
    if (!username_.empty() || !password_.empty()) {
      ++methods;
    }

    std::unique_ptr<nul::Buffer> reqBuf = nullptr;
    if (methods == 1) {
      const char req[] = { '\5', '\1', '\0' };
      reqBuf = bufferPool_->assembleDataBuffer(req, sizeof(req));

    } else {
      // \2 = username/password auth
      const char req[] = { '\5', '\2', '\0', '\2' };
      reqBuf = bufferPool_->assembleDataBuffer(req, sizeof(req));
    }
    conn_->writeAsync(std::move(reqBuf));
  }

  void SocksClient::sendSocksRequest(
    const std::string &targetHost, uint16_t targetPort) {
    uvcpp::SockAddrStorage sas;

    Socks::AddressType atyp = Socks::AddressType::UNKNOWN;

    if (nul::NetUtil::isIPv4(targetHost)) {
      atyp = Socks::AddressType::IPV4;
      nul::NetUtil::ipv4ToBinary(
        targetHost, reinterpret_cast<uint8_t *>(
          &reinterpret_cast<uvcpp::SockAddr4 *>(&sas)->sin_addr));

    } else if (nul::NetUtil::isIPv6(targetHost)) {
      atyp = Socks::AddressType::IPV6;
      nul::NetUtil::ipv6ToBinary(
        targetHost, reinterpret_cast<uint8_t *>(
          &reinterpret_cast<uvcpp::SockAddr6 *>(&sas)->sin6_addr));

    } else {
      atyp = Socks::AddressType::DOMAIN_NAME;
    }

    auto reqBuf = std::string{"\5\1\0", 3};  // send CONNECT request
    switch(atyp) {
      case Socks::AddressType::IPV4: {
        auto sa4 = reinterpret_cast<uvcpp::SockAddr4 *>(&sas);
        reqBuf.append(1, '\1');
        reqBuf.append(reinterpret_cast<const char *>(&sa4->sin_addr), 4);
        break;
      }
      case Socks::AddressType::DOMAIN_NAME: {
        reqBuf.append(1, '\3');
        reqBuf.append(1, static_cast<char>(targetHost.length()));
        reqBuf.append(targetHost);
        break;
      }
      case Socks::AddressType::IPV6: {
        auto sa6 = reinterpret_cast<uvcpp::SockAddr6 *>(&sas);
        reqBuf.append(1, '\4');
        reqBuf.append(reinterpret_cast<const char *>(&sa6->sin6_addr), 16);
        break;
      }
      default:
        break;
    }
    auto port = htons(targetPort);
    reqBuf.append(reinterpret_cast<const char *>(&port), 2);

    conn_->writeAsync(
      bufferPool_->assembleDataBuffer(reqBuf.c_str(), reqBuf.length()));
  }

  void SocksClient::writeAsync(std::unique_ptr<nul::Buffer> &&buffer) {
    if (conn_) {
      conn_->writeAsync(std::forward<std::unique_ptr<nul::Buffer>>(buffer));
    }
  }

  void SocksClient::close() {
    if (conn_) {
      conn_->close();
    }
  }

  void SocksClient::setUsername(const std::string &username) {
    username_ = username;
  }

  void SocksClient::setPassword(const std::string &password) {
    password_ = password;
  }

} /* end of namspace: sockapp */
