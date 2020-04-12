/*******************************************************************************
**          File: http_proxy_server.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 10:10 AM
**   Description: Http Proxy Daemon 
*******************************************************************************/
#ifndef PROXYPP_HTTP_PROXY_SERVER_H_
#define PROXYPP_HTTP_PROXY_SERVER_H_ 
#include <string>
#include <functional>

namespace proxypp {
  class HttpProxyServer final {
    public:
      enum class ServerStatus {
        STARTED,
        SHUTDOWN,
        ERROR_OCCURRED
      };
      using EventCallback =
        std::function<void(ServerStatus event, const std::string& message)>;
      
      HttpProxyServer();
      ~HttpProxyServer();
      bool start(const std::string &addr, uint16_t port, int backlog = 100);
      void shutdown();
      bool isRunning();

      void setEventCallback(EventCallback &&callback);

      // socks5://127.0.0.1:1080
      // http://127.0.0.1:8080
      void setUpstreamServer(const std::string &uriStr);

      std::size_t setAutoProxyRulesFile(const std::string &proxyRulesFile);
      std::size_t addAutoProxyRulesFile(const std::string &proxyRulesFile);
      bool addProxyRule(const std::string &rule);
      bool removeProxyRule(const std::string &rule);
      void clearProxyRules();
    
    private:
      void *ctx_{nullptr};
  };
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_HTTP_PROXY_SERVER_H_ */
