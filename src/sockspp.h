/*******************************************************************************
**          File: sockspp.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-27 Fri 05:24 PM
**   Description: the sockspp interface 
*******************************************************************************/
#ifndef SOCKSPP_H_
#define SOCKSPP_H_
#include <string>
#include <functional>

namespace sockspp {
  class Sockspp final {
    public:
      enum class ServerStatus {
        STARTED,
        SHUTDOWN,
        ERROR_OCCURRED
      };
      using EventCallback =
        std::function<void(ServerStatus event, const std::string& message)>;
      
      ~Sockspp();
      bool start(const std::string &addr, uint16_t port, int backlog = 100);
      void shutdown();
      bool isRunning();

      void setEventCallback(EventCallback &&callback);
      void setUsername(const std::string &username);
      void setPassword(const std::string &password);
    
    private:
      void *server_{nullptr};
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_H_ */
