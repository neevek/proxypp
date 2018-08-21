/*******************************************************************************
**          File: socks_server.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 05:37 PM
**   Description: see the header 
*******************************************************************************/
#include "sockspp/socks/socks_server.h"

namespace sockspp {

  SocksServer::SocksServer(const std::shared_ptr<uvcpp::Loop> &loop) :
    server_(uvcpp::Tcp::createUnique(loop)) {
  }

  bool SocksServer::start(const std::string &addr, Port port, int backlog) {
    bufferPool_ = std::make_shared<nul::BufferPool>(300, 60);
    if (server_) {
      server_->on<uvcpp::EvError>([this, addr, port](const auto &e, auto &server) {
        LOG_E("SocksServer failed to bind on %s:%d", addr.c_str(), port);
        if (this->eventCallback_) {
          this->eventCallback_(
            ServerStatus::ERROR_OCCURRED, std::string{uv_strerror(e.status)});
        }
      });
      server_->on<uvcpp::EvClose>([this, addr, port](const auto &e, auto &server) {
        LOG_D("SocksServer [%s:%d] closed", addr.c_str(), port);
        if (this->eventCallback_) {
          this->eventCallback_(ServerStatus::SHUTDOWN, "SocksServer shutdown");
        }
      });
      server_->on<uvcpp::EvAccept<uvcpp::Tcp>>([this](const auto &e, auto &server) {
        this->onClientConnected(
          std::move(const_cast<uvcpp::EvAccept<uvcpp::Tcp> &>(e).client));
      });

      if (server_->bind(addr, port) && server_->listen(backlog)) {
        LOG_I("SocksServer bound on %s:%d", addr.c_str(), port);
        if (this->eventCallback_) {
          this->eventCallback_(
            ServerStatus::STARTED,
            "SocksServer bound on " + addr + ":" + std::to_string(port));
        }
        return true;
      }
    }
    return false;
  }

  void SocksServer::onClientConnected(std::unique_ptr<uvcpp::Tcp> conn) {
    auto clientId = getNextClientId();
    conn->on<uvcpp::EvClose>([this, clientId](const auto &e, auto &conn) {
      this->removeClient(clientId);
    });

    auto client = std::make_shared<Client>(std::move(conn), bufferPool_);
    client->setUsername(username_);
    client->setPassword(password_);

    clients_[clientId] = client;
    client->start();

    LOG_D("Client count: %zu", clients_.size());
  }

  void SocksServer::shutdown() {
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

    for (auto &it : clients_) {
      it.second->close();
    }
    clients_.clear();
  }

  bool SocksServer::isRunning() const {
    return server_ && server_->isValid();
  }

  void SocksServer::setEventCallback(EventCallback &&callback) {
    eventCallback_ = callback;
  }

  void SocksServer::setUsername(const std::string &username) {
    username_ = username;
  }

  void SocksServer::setPassword(const std::string &password) {
    password_ = password;
  }

  void SocksServer::removeClient(Client::Id clientId) {
    auto clientIt = clients_.find(clientId);
    if (clientIt != clients_.end()) {
      auto client = std::move(clientIt->second);
      clients_.erase(clientIt);
    }
  }

  Client::Id SocksServer::getNextClientId() {
    return ++clientId_;
  }
  
} /* end of namspace: sockspp */
