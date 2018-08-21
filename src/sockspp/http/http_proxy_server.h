/*******************************************************************************
**          File: http_proxy_server.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 05:31 PM
**   Description: implementation of HTTP tunnel
*******************************************************************************/
#ifndef HTTP_PROXY_SERVER_H_
#define HTTP_PROXY_SERVER_H_
#include "sockspp/proxy_server.h"
#include "sockspp/http/http_conn.h"
#include "nul/buffer_pool.hpp"
#include "uvcpp.h"
#include <map>

namespace sockspp {

  class HttpProxyServer final : public ProxyServer {
    public:
      HttpProxyServer(const std::shared_ptr<uvcpp::Loop> &loop);
      bool start(const std::string &addr, Port port, int backlog = 100) override;
      void shutdown() override;
      bool isRunning() const override;
      void setEventCallback(EventCallback &&callback) override;

    private:
      void onClientConnected(std::unique_ptr<uvcpp::Tcp> conn);
      void removeConn(HttpConn::Id connId);
      HttpConn::Id getNextConnId();
    
    private:
      std::unique_ptr<uvcpp::Tcp> server_{nullptr};
      std::shared_ptr<nul::BufferPool> bufferPool_{nullptr};
      std::map<HttpConn::Id, std::shared_ptr<HttpConn>> connections_;
      EventCallback eventCallback_{nullptr};
      HttpConn::Id connId{0};
  };
  
} /* end of namspace: spp */

#endif /* end of include guard: HTTP_PROXY_SERVER_H_ */
