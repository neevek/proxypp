/*******************************************************************************
**          File: socks_proxy_server
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-27 Fri 05:41 PM
**   Description: see the header file 
*******************************************************************************/
#include "sockspp/socks/socks_proxy_server.h"
#include "sockspp/socks/socks_proxy_session.h"
#include "sockspp/proxy_server.hpp"

namespace {
  struct SocksProxyServerContext {
    std::unique_ptr<sockspp::ProxyServer> server;
    std::string username;
    std::string password;
  };
}

namespace sockspp {
  SocksProxyServer::~SocksProxyServer() {
    delete reinterpret_cast<SocksProxyServerContext *>(ctx_);
  }

  bool SocksProxyServer::start(const std::string &addr, uint16_t port, int backlog) {
    auto loop = std::make_shared<uvcpp::Loop>();
    if (!loop->init()) {
      LOG_E("Failed to start event loop");
      return false;
    }

    ctx_ = new SocksProxyServerContext{};

    auto ctx = reinterpret_cast<SocksProxyServerContext *>(ctx_);
    ctx->server = std::make_unique<ProxyServer>(loop);
    ctx->server->setSessionCreator([ctx](std::unique_ptr<uvcpp::Tcp> &&conn,
       const std::shared_ptr<nul::BufferPool> &bufferPool) {
      auto sess = std::make_shared<SocksProxySession>(std::move(conn), bufferPool);
      sess->setUsername(ctx->username);
      sess->setPassword(ctx->password);
      return sess;
    });

    if (!ctx->server->start(addr, port, backlog)) {
      LOG_E("Failed to start start SocksProxyServerContext");
      return false;
    }
    loop->run();
    return true;
  }

  void SocksProxyServer::shutdown() {
    if (ctx_) {
      reinterpret_cast<SocksProxyServerContext *>(ctx_)->server->shutdown();
    }
  }

  bool SocksProxyServer::isRunning() {
    return ctx_ &&
      reinterpret_cast<SocksProxyServerContext *>(ctx_)->server->isRunning();
  }

  void SocksProxyServer::setEventCallback(EventCallback &&callback) {
    if (ctx_) {
      reinterpret_cast<SocksProxyServerContext *>(ctx_)->server->
        setEventCallback([callback](auto status, auto &message){
        callback(static_cast<ServerStatus>(status), message);
      });
    }
  }

  void SocksProxyServer::setUsername(const std::string &username) {
    if (ctx_) {
      reinterpret_cast<SocksProxyServerContext *>(ctx_)->username = username;
    }
  }

  void SocksProxyServer::setPassword(const std::string &password) {
    if (ctx_) {
      reinterpret_cast<SocksProxyServerContext *>(ctx_)->password = password;
    }
  }
  
} /* end of namspace: sockspp */


#ifdef BUILD_CLIENT 
#include "sockspp/cli/cmdline.h"

int main(int argc, char *argv[]) {
  cmdline::parser p;

  p.add<std::string>("host", 'h', "IPv4 or IPv6 address", false, "0.0.0.0");
  p.add<uint16_t>(
    "port", 'p', "port number", true, 0, cmdline::range(1, 65535));
  p.add<int>("backlog", 'b', "backlog for the server", false, 200, cmdline::range(1, 65535));
  p.add<std::string>("username", 'U', "username", false);
  p.add<std::string>("password", 'P', "password", false);

  p.parse_check(argc, argv);

  sockspp::SocksProxyServer s{};
  s.setUsername(p.get<std::string>("username"));
  s.setPassword(p.get<std::string>("password"));
  s.start(
    p.get<std::string>("host"),
    p.get<uint16_t>("port"),
    p.get<int>("backlog"));
  return 0;
}
#endif
