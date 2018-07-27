/*******************************************************************************
**          File: socks_server.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 05:37 PM
**   Description: see the header 
*******************************************************************************/
#include "socks_server.h"

namespace sockspp {
  SocksServer::SocksServer() :
    bufferPool_(std::make_shared<BufferPool>(300, 60)) {
  }

  bool SocksServer::start(
    const std::shared_ptr<uvcpp::Loop> &loop,
    const std::string &host,
    Port port,
    int backlog) {

    if (server_) {
      LOG_E("SocksServer already started");
      return false;
    }

    server_ = uvcpp::Tcp::createUnique(loop);
    if (server_) {
      server_->on<uvcpp::EvError>([host, port](const auto &e, auto &server) {
        LOG_E("SocksServer failed to bind on %s:%d", host.c_str(), port);
      });
      server_->on<uvcpp::EvClose>([host, port](const auto &e, auto &server) {
        LOG_D("SocksServer [%s:%d] closed", host.c_str(), port);
      });
      server_->on<uvcpp::EvBind>([host, port, backlog](const auto &e, auto &server) {
        LOG_I("SocksServer bound on %s:%d", host.c_str(), port);
        server.listen(backlog);
      });
      server_->on<uvcpp::EvAccept<uvcpp::Tcp>>([this](const auto &e, auto &server) {
        this->onClientConnected(
          std::move(const_cast<uvcpp::EvAccept<uvcpp::Tcp> &>(e).client));
      });

      return server_->bind(host, port);
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
  
} /* end of namspace: sockspp */
