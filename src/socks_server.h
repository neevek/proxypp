/*******************************************************************************
**          File: socks_server.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 05:31 PM
**   Description: partial implementation of RFC-1928
*******************************************************************************/
#ifndef SOCKS_SERVER_H_
#define SOCKS_SERVER_H_
#include "uvcpp.h"
#include "client.h"
#include "buffer_pool.h"

namespace sockspp {

  class SocksServer {
    public:
      using Port = uint16_t;

      enum class ServerStatus {
        STARTED,
        SHUTDOWN,
        ERROR_OCCURRED
      };
      using ServerEventCallback =
        std::function<void(ServerStatus event, const std::string& message)>;

      SocksServer(const std::string &addr, Port port, int backlog = 100);
      bool start(const std::shared_ptr<uvcpp::Loop> &loop);
      void shutdown();
      bool isRunning() const;
      void setEventCallback(ServerEventCallback &&callback);

    private:
      void onClientConnected(std::unique_ptr<uvcpp::Tcp> conn);
    
    private:
      std::string addr_;
      Port port_;
      int backlog_;
      std::unique_ptr<uvcpp::Tcp> server_{nullptr};
      std::shared_ptr<BufferPool> bufferPool_{nullptr};
      ServerEventCallback eventCallback_{nullptr};
      uint32_t clientCount_{0};
  };
  
} /* end of namspace: spp */

#endif /* end of include guard: SOCKS_SERVER_H_ */
