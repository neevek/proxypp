/*******************************************************************************
**          File: socks_proxy_server.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-27 Fri 05:24 PM
**   Description: the proxypp interface 
*******************************************************************************/
#ifndef PROXYPP_SOCKS_PROXY_SERVER_H_
#define PROXYPP_SOCKS_PROXY_SERVER_H_
#include <string>
#include <functional>

namespace proxypp {
  class SocksProxyServer final {
    public:
      enum class ServerStatus {
        STARTED,
        SHUTDOWN,
        ERROR_OCCURRED
      };
      using EventCallback =
        std::function<void(ServerStatus event, const std::string& message)>;

      SocksProxyServer();
      ~SocksProxyServer();
      bool start(const std::string &addr, uint16_t port, int backlog = 100);
      void shutdown();
      bool isRunning();

      void setEventCallback(EventCallback &&callback);
      void setUsername(const std::string &username);
      void setPassword(const std::string &password);
    
    private:
      void *ctx_{nullptr};
  };
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_SOCKS_PROXY_SERVER_H_ */
