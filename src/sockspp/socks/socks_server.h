/*******************************************************************************
**          File: socks_server.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 05:31 PM
**   Description: partial implementation of RFC-1928
*******************************************************************************/
#ifndef SOCKS_SERVER_H_
#define SOCKS_SERVER_H_
#include "sockspp/proxy_server.h"
#include "sockspp/socks/socks_conn.h"
#include "nul/buffer_pool.hpp"
#include "uvcpp.h"
#include <map>

namespace sockspp {

  class SocksServer final : public ProxyServer {
    public:
      SocksServer(const std::shared_ptr<uvcpp::Loop> &loop);
      bool start(const std::string &addr, Port port, int backlog = 100) override;
      void shutdown() override;
      bool isRunning() const override;
      void setEventCallback(EventCallback &&callback) override;

      void setUsername(const std::string &username);
      void setPassword(const std::string &password);

    private:
      void onClientConnected(std::unique_ptr<uvcpp::Tcp> conn);
      void removeConn(SocksConn::Id connId);
      SocksConn::Id getNextConnId();
    
    private:
      std::unique_ptr<uvcpp::Tcp> server_{nullptr};
      std::shared_ptr<nul::BufferPool> bufferPool_{nullptr};
      std::map<SocksConn::Id, std::shared_ptr<SocksConn>> connections_;
      EventCallback eventCallback_{nullptr};
      SocksConn::Id connId{0};

      std::string username_;
      std::string password_;
  };
  
} /* end of namspace: spp */

#endif /* end of include guard: SOCKS_SERVER_H_ */
