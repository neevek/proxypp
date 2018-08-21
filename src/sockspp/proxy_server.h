/*******************************************************************************
**          File: http_proxy_server.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-21 Tue 09:41 AM
**   Description: proxy server interface
*******************************************************************************/
#ifndef PROXY_SERVER_H_
#define PROXY_SERVER_H_
#include <string>
#include <functional>

namespace sockspp {
  class ProxyServer {
    public:
      using Port = uint16_t;

      enum class ServerStatus {
        STARTED,
        SHUTDOWN,
        ERROR_OCCURRED
      };
      using EventCallback =
        std::function<void(ServerStatus event, const std::string& message)>;

      virtual ~ProxyServer() = default;
      virtual bool start(
        const std::string &addr, Port port, int backlog = 100) = 0;
      virtual void shutdown() = 0;
      virtual bool isRunning() const = 0;

      virtual void setEventCallback(EventCallback &&callback) = 0;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: PROXY_SERVER_H_ */

