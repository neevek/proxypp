/*******************************************************************************
**          File: http_proxy_server.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 10:12 AM
**   Description: see the header file 
*******************************************************************************/
#include "sockspp/http/http_proxy_server.h"
#include "sockspp/http/http_proxy_session.h"
#include "sockspp/proxy_server.hpp"

namespace {
  struct HttpProxyServerContext {
    sockspp::ProxyServer server;
    std::string socksServerHost;
    uint16_t socksServerPort;
  };
}

namespace sockspp {
  HttpProxyServer::HttpProxyServer() : ctx_(new HttpProxyServerContext()) {
  }

  HttpProxyServer::~HttpProxyServer() {
    delete reinterpret_cast<HttpProxyServerContext *>(ctx_);
  }

  bool HttpProxyServer::start(const std::string &addr, uint16_t port, int backlog) {
    auto loop = std::make_shared<uvcpp::Loop>();
    if (!loop->init()) {
      LOG_E("Failed to start event loop");
      return false;
    }

    auto ctx = reinterpret_cast<HttpProxyServerContext *>(ctx_);
    ctx->server.setSessionCreator(
      [ctx](std::unique_ptr<uvcpp::Tcp> &&conn,
         const std::shared_ptr<nul::BufferPool> &bufferPool) {
        auto sess =
          std::make_shared<HttpProxySession>(std::move(conn), bufferPool);
        sess->setUpstreamSocksServer(ctx->socksServerHost, ctx->socksServerPort);
        return sess;
      });

    if (!ctx->server.start(loop, addr, port, backlog)) {
      LOG_E("Failed to start start HttpProxyServerContext");
      return false;
    }
    loop->run();
    return true;
  }

  void HttpProxyServer::shutdown() {
    if (ctx_) {
      reinterpret_cast<HttpProxyServerContext *>(ctx_)->server.shutdown();
    }
  }

  bool HttpProxyServer::isRunning() {
    return ctx_ &&
      reinterpret_cast<HttpProxyServerContext *>(ctx_)->server.isRunning();
  }

  void HttpProxyServer::setEventCallback(EventCallback &&callback) {
    if (ctx_) {
      reinterpret_cast<HttpProxyServerContext *>(ctx_)->server.
        setEventCallback([callback](auto status, auto &message){
        callback(static_cast<ServerStatus>(status), message);
      });
    }
  }

  void HttpProxyServer::setUpstreamSocksServer(
    const std::string &ip, uint16_t port) {
    if (ctx_) {
      auto ctx = reinterpret_cast<HttpProxyServerContext *>(ctx_);
      ctx->socksServerHost = ip;
      ctx->socksServerPort = port;
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
  p.add<std::string>(
    "socks_server_host", 'H', "IPv4 or IPv6 address", false, "0.0.0.0");
  p.add<uint16_t>(
    "socks_server_port", 'P', "port number", false, 0, cmdline::range(1, 65535));

  p.parse_check(argc, argv);

  sockspp::HttpProxyServer d{};
  d.setUpstreamSocksServer(
    p.get<std::string>("socks_server_host"),
    p.get<uint16_t>("socks_server_port"));

  d.start(
    p.get<std::string>("host"),
    p.get<uint16_t>("port"),
    p.get<int>("backlog"));
  return 0;
}
#endif
