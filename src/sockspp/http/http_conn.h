/*******************************************************************************
**          File: http_conn.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:03 PM
**   Description: object that wraps the tcp connection to the HTTP proxy server
**                and connection to the remote         
*******************************************************************************/
#ifndef HTTP_CONN_H_
#define HTTP_CONN_H_
#include "sockspp/conn.h"
#include "uvcpp.h"
#include "nul/buffer_pool.hpp"

namespace sockspp {
  /**
   * This class MUST be used with std::shared_ptr
   */
  class HttpConn final :
    public Conn, public std::enable_shared_from_this<HttpConn> {

    public:
      HttpConn(
        std::unique_ptr<uvcpp::Tcp> &&conn,
        const std::shared_ptr<nul::BufferPool> &bufferPool);
      virtual void start() override;
      virtual void close() override;

    private:
      void replyDownstream(const std::string &message);
      void connectUpstreamWithAddr(const std::string &host, uint16_t port);
      void connectUpstreamWithIp(const std::string &ip, uint16_t port);
      void createUpstreamConnection(uint16_t port);
    
    private:
      std::unique_ptr<uvcpp::Tcp> downstreamConn_{nullptr};
      std::shared_ptr<uvcpp::Tcp> upstreamConn_{nullptr};
      std::unique_ptr<uvcpp::DNSRequest> dnsRequest_{nullptr};
      uvcpp::EvDNSResult::DNSResultVector ipAddrs_;
      decltype(ipAddrs_.begin()) ipIt_{ipAddrs_.end()};
      bool upstreamConnected_{false};

      std::shared_ptr<nul::BufferPool> bufferPool_;

      std::string requestData_;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: HTTP_CONN_H_ */
