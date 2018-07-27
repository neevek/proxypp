/*******************************************************************************
**          File: sockspp.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-27 Fri 05:41 PM
**   Description: see the header file 
*******************************************************************************/
#include "sockspp.h"
#include "socks_server.h"

namespace sockspp {
  Sockspp::Sockspp(const std::string &addr, uint16_t port, int backlog) {
    server_ = new SocksServer(addr, port, backlog);
  }

  Sockspp::~Sockspp() {
    delete reinterpret_cast<SocksServer *>(server_);
  }

  bool Sockspp::start() {
    auto loop = std::make_shared<uvcpp::Loop>();
    if (!loop->init()) {
      LOG_E("Failed to start event loop");
      return false;
    }
    if (!reinterpret_cast<SocksServer *>(server_)->start(loop)) {
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
  
} /* end of namspace: sockspp */


#ifdef BUILD_CLIENT 
int main(int argc, const char *argv[]) {
  sockspp::Sockspp s{"0.0.0.0", 9888, 200};
  s.start();
  return 0;
}
#endif
