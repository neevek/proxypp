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

      SocksServer();
      bool start(const std::shared_ptr<uvcpp::Loop> &loop,
                 const std::string &addr,
                 Port port,
                 int backlog = 100);
      void shutdown();

    private:
      void onClientConnected(std::unique_ptr<uvcpp::Tcp> conn);
    
    private:
      std::unique_ptr<uvcpp::Tcp> server_{nullptr};
      std::shared_ptr<BufferPool> bufferPool_{nullptr};

      uint32_t clientCount_{0};
  };
  
} /* end of namspace: spp */

#endif /* end of include guard: SOCKS_SERVER_H_ */
