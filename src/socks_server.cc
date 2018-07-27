/*******************************************************************************
**          File: socks_server.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 05:37 PM
**   Description: see the header 
*******************************************************************************/
#include "socks_server.h"

namespace sockspp {

  SocksServer::SocksServer(const std::string &addr, Port port, int backlog) :
    addr_(addr), port_(port), backlog_(backlog) {
  }

  bool SocksServer::start(const std::shared_ptr<uvcpp::Loop> &loop) {
    if (server_) {
      LOG_E("SocksServer already started");
      return false;
    }

    bufferPool_ = std::make_shared<BufferPool>(300, 60);
    server_ = uvcpp::Tcp::createUnique(loop);
    if (server_) {
      server_->on<uvcpp::EvError>([this](const auto &e, auto &server) {
        LOG_E("SocksServer failed to bind on %s:%d", addr_.c_str(), port_);
        if (this->eventCallback_) {
          this->eventCallback_(
            ServerStatus::ERROR_OCCURRED, std::string{uv_strerror(e.status)});
        }
      });
      server_->on<uvcpp::EvClose>([this](const auto &e, auto &server) {
        LOG_D("SocksServer [%s:%d] closed", addr_.c_str(), port_);
        if (this->eventCallback_) {
          this->eventCallback_(ServerStatus::SHUTDOWN, "SocksServer shutdown");
        }
      });
      server_->on<uvcpp::EvBind>(
        [this](const auto &e, auto &server) {
        LOG_I("SocksServer bound on %s:%d", addr_.c_str(), port_);
        server.listen(backlog_);

        if (this->eventCallback_) {
          this->eventCallback_(
            ServerStatus::STARTED,
            "SocksServer bound on " + addr_ + ":" + std::to_string(port_));
        }
      });
      server_->on<uvcpp::EvAccept<uvcpp::Tcp>>([this](const auto &e, auto &server) {
        this->onClientConnected(
          std::move(const_cast<uvcpp::EvAccept<uvcpp::Tcp> &>(e).client));
      });

      return server_->bind(addr_, port_);
    }
    return false;
  }

  void SocksServer::onClientConnected(std::unique_ptr<uvcpp::Tcp> conn) {
    ++clientCount_;
    conn->on<uvcpp::EvClose>([this](const auto &e, auto &conn) {
      --clientCount_;
    });

    std::make_shared<Client>(std::move(conn), bufferPool_)->start();
    LOG_D("Client count: %d", clientCount_);
  }

  void SocksServer::shutdown() {
    if (server_) {
      server_->close();
    }
  }

  bool SocksServer::isRunning() const {
    return server_ && server_->isValid();
  }

  void SocksServer::setEventCallback(ServerEventCallback &&callback) {
    eventCallback_ = callback;
  }
  
} /* end of namspace: sockspp */
