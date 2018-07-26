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
#include <map>

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
      std::unique_ptr<Client> removeClient(Client::Id clientId);

      Client::Id getNextClientId();
    
    private:
      std::unique_ptr<uvcpp::Tcp> server_{nullptr};
      std::map<Client::Id, std::unique_ptr<Client>> clients_;
      std::shared_ptr<BufferPool> bufferPool_{nullptr};

      Client::Id clientId_{0};
  };
  
} /* end of namspace: spp */

#endif /* end of include guard: SOCKS_SERVER_H_ */
