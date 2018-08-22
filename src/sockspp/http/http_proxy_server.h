/*******************************************************************************
**          File: d.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 10:10 AM
**   Description: Http Proxy Daemon 
*******************************************************************************/
#ifndef SOCKSPP_HTTP_PROXY_SERVER_H_
#define SOCKSPP_HTTP_PROXY_SERVER_H_ 
#include <string>
#include <functional>

namespace sockspp {
  class HttpProxyServer final {
    public:
      enum class ServerStatus {
        STARTED,
        SHUTDOWN,
        ERROR_OCCURRED
      };
      using EventCallback =
        std::function<void(ServerStatus event, const std::string& message)>;
      
      ~HttpProxyServer();
      bool start(const std::string &addr, uint16_t port, int backlog = 100);
      void shutdown();
      bool isRunning();

      void setEventCallback(EventCallback &&callback);
    
    private:
      void *server_{nullptr};
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_HTTP_PROXY_SERVER_H_ */
