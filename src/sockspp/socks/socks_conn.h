/*******************************************************************************
**          File: socks_conn.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:03 PM
**   Description: object that wraps the tcp connection to the SOCKS server
**                and connection to the remote         
*******************************************************************************/
#ifndef SOCKS_CONN_H_
#define SOCKS_CONN_H_
#include "uvcpp.h"
#include "sockspp/socks/socks_req_parser.h"
#include "nul/buffer_pool.hpp"

namespace sockspp {
  /**
   * This class MUST be used with std::shared_ptr
   */
  class SocksConn : public std::enable_shared_from_this<SocksConn> {
    public:
      using Id = uint32_t;

      SocksConn(std::unique_ptr<uvcpp::Tcp> &&conn,
             const std::shared_ptr<nul::BufferPool> &bufferPool);
      void start();
      void close();
      void setUsername(const std::string &username);
      void setPassword(const std::string &password);

    private:
      void replySocksError();
      void connectUpstream();
      void connectUpstream(uvcpp::SockAddr *sockAddr);
      void connectUpstream(const std::string &ip);
      void createUpstreamConnection();
    
    private:
      std::unique_ptr<uvcpp::Tcp> downstreamConn_{nullptr};
      std::shared_ptr<uvcpp::Tcp> upstreamConn_{nullptr};
      std::unique_ptr<uvcpp::DNSRequest> dnsRequest_{nullptr};
      uvcpp::EvDNSResult::DNSResultVector ipAddrs_;
      decltype(ipAddrs_.begin()) ipIt_{ipAddrs_.end()};
      bool upstreamConnected_{false};

      std::shared_ptr<nul::BufferPool> bufferPool_;

      SocksReqParser socks_;
      std::string username_;
      std::string password_;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKS_CONN_H_ */
