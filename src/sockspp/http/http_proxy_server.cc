/*******************************************************************************
**          File: socks_server.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 05:37 PM
**   Description: see the header 
*******************************************************************************/
#include "sockspp/http/http_proxy_server.h"

namespace sockspp {

  HttpProxyServer::HttpProxyServer(const std::shared_ptr<uvcpp::Loop> &loop) :
    server_(uvcpp::Tcp::createUnique(loop)) {
  }

  bool HttpProxyServer::start(const std::string &addr, Port port, int backlog) {
    bufferPool_ = std::make_shared<nul::BufferPool>(300, 60);
    if (server_) {
      server_->on<uvcpp::EvError>([this, addr, port](const auto &e, auto &server) {
        LOG_E("HttpProxyServer failed to bind on %s:%d", addr.c_str(), port);
        if (this->eventCallback_) {
          this->eventCallback_(
            ServerStatus::ERROR_OCCURRED, std::string{uv_strerror(e.status)});
        }
      });
      server_->on<uvcpp::EvClose>([this, addr, port](const auto &e, auto &server) {
        LOG_D("HttpProxyServer [%s:%d] closed", addr.c_str(), port);
        if (this->eventCallback_) {
          this->eventCallback_(ServerStatus::SHUTDOWN, "HttpProxyServer shutdown");
        }
      });
      server_->on<uvcpp::EvAccept<uvcpp::Tcp>>([this](const auto &e, auto &server) {
        this->onClientConnected(
          std::move(const_cast<uvcpp::EvAccept<uvcpp::Tcp> &>(e).client));
      });

      if (server_->bind(addr, port) && server_->listen(backlog)) {
        LOG_I("HttpProxyServer bound on %s:%d", addr.c_str(), port);
        if (this->eventCallback_) {
          this->eventCallback_(
            ServerStatus::STARTED,
            "HttpProxyServer bound on " + addr + ":" + std::to_string(port));
        }
        return true;
      }
    }
    return false;
  }

  void HttpProxyServer::onClientConnected(std::unique_ptr<uvcpp::Tcp> conn) {
    auto connId = getNextConnId();
    conn->on<uvcpp::EvClose>([this, connId](const auto &e, auto &conn) {
      this->removeConn(connId);
    });

    auto client = std::make_shared<HttpConn>(std::move(conn), bufferPool_);
    connections_[connId] = client;
    client->start();

    LOG_D("HttpConn count: %zu", connections_.size());
  }

  void HttpProxyServer::shutdown() {
    if (server_) {
      // must shutdown the Server from inside the loop
      auto work = uvcpp::Work::createShared(server_->getLoop());
      work->ref(work);
      work->once<uvcpp::EvWork>([this](const auto &e, auto &work) {
        server_->close();
        work.unrefAll();
      });
      work->start();
    }

    for (auto &it : connections_) {
      it.second->close();
    }
    connections_.clear();
  }

  bool HttpProxyServer::isRunning() const {
    return server_ && server_->isValid();
  }

  void HttpProxyServer::setEventCallback(EventCallback &&callback) {
    eventCallback_ = callback;
  }

  void HttpProxyServer::removeConn(HttpConn::Id connId) {
    auto clientIt = connections_.find(connId);
    if (clientIt != connections_.end()) {
      auto client = std::move(clientIt->second);
      connections_.erase(clientIt);
    }
  }

  HttpConn::Id HttpProxyServer::getNextConnId() {
    return ++connId;
  }
  
} /* end of namspace: sockspp */
