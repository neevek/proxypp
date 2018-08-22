/*******************************************************************************
**          File: hpd.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 10:12 AM
**   Description: see the header file 
*******************************************************************************/
#include "sockspp/http/hpd.h"
#include "sockspp/http/http_conn.h"
#include "sockspp/proxy_server.hpp"

namespace sockspp {
  using HttpProxyServer = ProxyServer<HttpConn>;
  
  Hpd::~Hpd() {
    delete reinterpret_cast<HttpProxyServer *>(server_);
  }

  bool Hpd::start(const std::string &addr, uint16_t port, int backlog) {
    auto loop = std::make_shared<uvcpp::Loop>();
    if (!loop->init()) {
      LOG_E("Failed to start event loop");
      return false;
    }

    server_ = new HttpProxyServer(loop);
    auto server = reinterpret_cast<HttpProxyServer *>(server_);
    server->setConnCreator(
      [](std::unique_ptr<uvcpp::Tcp> &&tcpConn,
         const std::shared_ptr<nul::BufferPool> &bufferPool) {
        return std::make_shared<HttpConn>(std::move(tcpConn), bufferPool);
      });

    if (!server->start(addr, port, backlog)) {
      LOG_E("Failed to start start HttpProxyServer");
      return false;
    }
    loop->run();
    return true;
  }

  void Hpd::shutdown() {
    if (server_) {
      reinterpret_cast<HttpProxyServer *>(server_)->shutdown();
    }
  }

  bool Hpd::isRunning() {
    return server_ &&
      reinterpret_cast<HttpProxyServer *>(server_)->isRunning();
  }

  void Hpd::setEventCallback(EventCallback &&callback) {
    if (server_) {
      reinterpret_cast<HttpProxyServer *>(server_)->
        setEventCallback([callback](auto status, auto &message){
        callback(static_cast<ServerStatus>(status), message);
      });
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

  p.parse_check(argc, argv);

  sockspp::Hpd d{};
  d.start(
    p.get<std::string>("host"),
    p.get<uint16_t>("port"),
    p.get<int>("backlog"));
  return 0;
}
#endif
