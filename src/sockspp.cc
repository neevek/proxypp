/*******************************************************************************
**          File: sockspp.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-27 Fri 05:41 PM
**   Description: see the header file 
*******************************************************************************/
#include "sockspp.h"
#include "socks_server.h"

namespace sockspp {
  Sockspp::~Sockspp() {
    delete reinterpret_cast<SocksServer *>(server_);
  }

  bool Sockspp::start(const std::string &addr, uint16_t port, int backlog) {
    auto loop = std::make_shared<uvcpp::Loop>();
    if (!loop->init()) {
      LOG_E("Failed to start event loop");
      return false;
    }

    server_ = new SocksServer(loop);
    if (!reinterpret_cast<SocksServer *>(server_)->start(addr, port, backlog)) {
      LOG_E("Failed to start start SocksServer");
      return false;
    }
    loop->run();
    return true;
  }

  void Sockspp::shutdown() {
    if (server_) {
      reinterpret_cast<SocksServer *>(server_)->shutdown();
    }
  }

  bool Sockspp::isRunning() {
    return server_ &&
      reinterpret_cast<SocksServer *>(server_)->isRunning();
  }

  void Sockspp::setEventCallback(EventCallback &&callback) {
    if (server_) {
      reinterpret_cast<SocksServer *>(server_)->
        setEventCallback([callback](auto status, auto &message){
        callback(static_cast<ServerStatus>(status), message);
      });
    }
  }

  void Sockspp::setUsername(const std::string &username) {
    if (server_) {
      reinterpret_cast<SocksServer *>(server_)->setUsername(username);
    }
  }

  void Sockspp::setPassword(const std::string &password) {
    if (server_) {
      reinterpret_cast<SocksServer *>(server_)->setPassword(password);
    }
  }
  
} /* end of namspace: sockspp */


#ifdef BUILD_CLIENT 
#include "cli/cmdline.h"

int main(int argc, char *argv[]) {
  cmdline::parser p;

  p.add<std::string>("host", 'h', "IPv4 or IPv6 address", false, "0.0.0.0");
  p.add<uint16_t>(
    "port", 'p', "port number", true, 0, cmdline::range(1, 65535));
  p.add<int>("backlog", 'b', "backlog for the server", false, 200, cmdline::range(1, 65535));
  p.add<std::string>("username", 'U', "username", false);
  p.add<std::string>("password", 'P', "password", false);

  p.parse_check(argc, argv);

  sockspp::Sockspp s{
    p.get<std::string>("host"),
    p.get<uint16_t>("port"),
    p.get<int>("backlog"),
  };

  s.setUsername(p.get<std::string>("username"));
  s.setPassword(p.get<std::string>("password"));
  s.start();
  return 0;
}
#endif
