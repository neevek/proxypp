/*******************************************************************************
**          File: http_proxy_session.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:05 PM
**   Description: see the header file 
*******************************************************************************/
#include "proxypp/http/http_proxy_session.h"
#include "nul/log.hpp"
#include "nul/util.hpp"
#include "proxypp/http/http_header_parser.h"

#include <algorithm>
#include <cstring>

namespace {
  static const auto REPLY_BAD_REQUEST =
    std::string{"HTTP/1.1 400 Bad Request\r\nServer: hpd\r\n\r\n"};
  static const auto REPLY_BAD_GATEWAY =
    std::string{"HTTP/1.1 502 Bad Gateway\r\nServer: hpd\r\n\r\n"};
  static const auto REPLY_OK_FOR_CONNECT_REQUEST =
    std::string{"HTTP/1.1 200 OK\r\nServer: hpd\r\n\r\n"};
  static const auto HTTP_HEADER_PROXY_CONNECTION =
    std::string{"Proxy-Connection"};
}

namespace proxypp {
  HttpProxySession::HttpProxySession(
    const std::shared_ptr<uvcpp::Tcp> &conn,
    const std::shared_ptr<nul::BufferPool> &bufferPool) :
    downstreamConn_(std::move(conn)), bufferPool_(bufferPool) {
  }

  void HttpProxySession::start() {
    downstreamConn_->once<uvcpp::EvClose>(
      // intentionally cycle-ref the HttpProxySession object to avoid
      // deletion of it before this callback is fired
      [this, _ = shared_from_this()](const auto &e, auto &client){

      ipIt_ = ipAddrs_.end();
      if (upstreamConn_) {
        upstreamConn_->close();
      } else if (socksClient_) {
        socksClient_->close();
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
        if (upstreamConn_) {
          upstreamConn_->writeAsync(
            bufferPool_->assembleDataBuffer(e.buf, e.nread));

        } else if (socksClient_) {
          socksClient_->writeAsync(
            bufferPool_->assembleDataBuffer(e.buf, e.nread));

        } else {
          // not possible to reach here
          abort();
        }
        return;
      }

      requestData_.append(e.buf, e.nread);
      if (hasReadHeader_) {
        LOG_W("has already read enough data for header");
        return;
      }

      auto pos = HttpHeaderParser::findHeaderEndPos(requestData_);
      if (pos == std::string::npos) {
        if (!HttpHeaderParser::startsWithValidHttpMethod(requestData_)) {
          LOG_E("request does not starts with valid http method");
          this->replyDownstream(REPLY_BAD_REQUEST);
          conn.close();
          return;
        }

        LOG_D("expecting more data for the header: %zu", requestData_.size());
        return;
      }

      hasReadHeader_ = true;
      HttpHeaderParser parser;
      std::string addr;
      uint16_t port;

      if (!parser.parse(requestData_, pos) ||
          !parser.getAddrAndPort(addr, port)) {
        this->replyDownstream(REPLY_BAD_REQUEST);
        conn.close();
        return;
      }

      // not sure if we need the request data at this point,
      // so save as a temp variable here
      auto tempRequestData = std::move(requestData_);
      if (!parser.isConnectMethod()) {
        std::swap(requestData_, tempRequestData);
      }

      if (upstreamType_ != UpstreamType::kUnknown &&
          (!proxyRuleManager_ || proxyRuleManager_->matches(addr, port))) {

        if (upstreamType_ == UpstreamType::kSOCKS5) {
          this->initiateSocksConnection(addr, port);

        } else if (upstreamType_ == UpstreamType::kHTTP) {
          // redirect all the request data to the remote HTTP proxy server
          // regardless of whether it is a CONNECT request
          if (!tempRequestData.empty()) {
            std::swap(requestData_, tempRequestData);
          }
          this->connectUpstreamWithAddr(
            upstreamServerHost_, upstreamServerPort_);

        } else {
          // DIRECT connect if no proxy server is available
          // Should not reach here!
          this->connectUpstreamWithAddr(addr, port);
        }

      } else {
        this->connectUpstreamWithAddr(addr, port);
      }
      });

    downstreamConn_->readStart();
  }

  void HttpProxySession::initiateSocksConnection(
    const std::string &targetServerAddr, uint16_t targetServerPort) {
    socksClient_ =
      std::make_unique<SocksClient>(downstreamConn_->getLoop(), bufferPool_);

    if (!socksClient_->connect(upstreamServerHost_, upstreamServerPort_)) {
      replyDownstream(REPLY_BAD_GATEWAY);
      socksClient_->close();

    } else {
      // ref the session object until the SocksClient connection is closed
      socksClient_->once<uvcpp::EvClose>([this, _ = shared_from_this()](
          const auto &e, auto &conn){
        downstreamConn_->close();
      });

      socksClient_->once<uvcpp::EvError>(
        [this](const auto &e, auto &client) {
          if (!upstreamConnected_) {
            LOG_E("Failed to connect to SOCKS server: %s:%d",
                  client.getIP().c_str(), client.getPort());
            this->replyDownstream(REPLY_BAD_GATEWAY);
          }
        });

      socksClient_->once<EvSocksHandshake>([=](const auto &e, auto &conn){
        if (!e.succeeded) {
          this->replyDownstream(REPLY_BAD_GATEWAY);
          conn.close();
          return;
        }

        LOG_V("Connected to SOCKS server for target: %s:%d",
              targetServerAddr.c_str(), targetServerPort);
        this->onUpstreamConnected(conn);

        conn.template on<EvSocksRead>([this](const auto &e, auto &conn){
          downstreamConn_->writeAsync(
            bufferPool_->assembleDataBuffer(e.buf, e.nread));
        });
      });

      socksClient_->startHandshake(targetServerAddr, targetServerPort);
    }
  }

  void HttpProxySession::connectUpstreamWithAddr(const std::string &addr, uint16_t port) {
    if (nul::NetUtil::isIPv4(addr) || nul::NetUtil::isIPv6(addr)) {
      connectUpstreamWithIp(addr, port);
      return;
    }

    dnsRequest_ = uvcpp::DNSRequest::create(downstreamConn_->getLoop());
    dnsRequest_->once<uvcpp::EvDNSRequestFinish>(
      // intentionally cycle-ref the HttpProxySession object to avoid
      // deletion of it before this callback is fired
      [this, _ = shared_from_this()](const auto &e, auto &req){
        dnsRequest_ = nullptr;
      });

    dnsRequest_->once<uvcpp::EvError>([this, addr](const auto &e, auto &r) {
      LOG_W("Failed to resolve address: %s", addr.c_str());
      this->replyDownstream(REPLY_BAD_GATEWAY);
      downstreamConn_->close();
    });

    dnsRequest_->once<uvcpp::EvDNSResult>(
      [this, addr, port](const auto &e, auto &req) {
        if (e.dnsResults.empty()) {
          LOG_W("[%s] resolved to zero IPs", addr.c_str());
          this->replyDownstream(REPLY_BAD_GATEWAY);
          downstreamConn_->close();
          return;
        }

        ipAddrs_ = std::move(e.dnsResults);
        ipIt_ = ipAddrs_.begin();
        auto newIp = *ipIt_;
        ++ipIt_;

        if (downstreamConn_->isValid()) {
          this->connectUpstreamWithIp(newIp, port);
        }
      });

    dnsRequest_->resolve(addr);
    LOG_D("Resolving address: %s", addr.c_str());
  }

  void HttpProxySession::connectUpstreamWithIp(
    const std::string &ip, uint16_t port) {
    createUpstreamConnection(port);
    if (!upstreamConn_->connect(ip, port)) {
      // check if there're more IPs to try
      if (ipIt_ == ipAddrs_.end()) {
        replyDownstream(REPLY_BAD_GATEWAY);
      }
      upstreamConn_->close();
    }
  }

  void HttpProxySession::createUpstreamConnection(uint16_t port) {
    upstreamConn_ = uvcpp::Tcp::create(downstreamConn_->getLoop());
    upstreamConn_->once<uvcpp::EvError>(
      [this](const auto &e, auto &client) {
        if (!upstreamConnected_) {
          LOG_E("Failed to connect to: %s:%d",
                client.getIP().c_str(), client.getPort());
          this->replyDownstream(REPLY_BAD_GATEWAY);
        }
      });
    upstreamConn_->once<uvcpp::EvClose>(
      // intentionally cycle-ref the HttpProxySession object to avoid
      // deletion of it before this callback is fired
      [this, port, _ = shared_from_this()](const auto &e, auto &client){
      if (!upstreamConnected_ && ipIt_ != ipAddrs_.end()) {
        auto newIp = *ipIt_;
        ++ipIt_;
        this->connectUpstreamWithIp(newIp, port);

      } else {
        downstreamConn_->close();
      }
    });
    upstreamConn_->once<uvcpp::EvConnect>(
      [this](const auto &e, auto &conn) {
        LOG_V("Connected to: %s:%d", conn.getIP().c_str(), conn.getPort());
        this->onUpstreamConnected(*upstreamConn_);

        conn.template on<uvcpp::EvBufferRecycled>([this](const auto &e, auto &conn) {
          bufferPool_->returnBuffer(std::forward<std::unique_ptr<nul::Buffer>>(
              const_cast<uvcpp::EvBufferRecycled &>(e).buffer));
        });

        conn.template on<uvcpp::EvRead>([this](const auto &e, auto &conn){
          downstreamConn_->writeAsync(
            bufferPool_->assembleDataBuffer(e.buf, e.nread));
        });

        upstreamConn_->readStart();
      });
  }

  void HttpProxySession::onUpstreamConnected(uvcpp::Tcp &conn) {
    upstreamConnected_ = true;

    if (!requestData_.empty()) {
      auto pos = requestData_.find(HTTP_HEADER_PROXY_CONNECTION);
      if (pos != std::string::npos) {
        requestData_ = requestData_.replace(
          pos, HTTP_HEADER_PROXY_CONNECTION.length(), "Connection");
      }

      conn.writeAsync(bufferPool_->assembleDataBuffer(
          requestData_.c_str(), requestData_.length()));
      requestData_.clear();

    } else {
      replyDownstream(REPLY_OK_FOR_CONNECT_REQUEST);
    }
  }

  void HttpProxySession::replyDownstream(const std::string &message) {
    downstreamConn_->writeAsync(
      bufferPool_->assembleDataBuffer(message.c_str(), message.length()));
  }

  void HttpProxySession::close() {
    downstreamConn_->close();
  }

  void HttpProxySession::setUpstreamServer(
    UpstreamType upstreamType, const std::string &ip, uint16_t port) {
    upstreamType_ = upstreamType;
    upstreamServerHost_ = ip;
    upstreamServerPort_ = port;
  }

  void HttpProxySession::setAutoProxyManager(
    const std::shared_ptr<AutoProxyManager> &proxyRuleManager) {
    proxyRuleManager_ = proxyRuleManager;
  }
} /* end of namspace: proxypp */
