/*******************************************************************************
**          File: socks_proxy_session.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:03 PM
**   Description: object that wraps the tcp connection to the SOCKS server
**                and connection to the remote         
*******************************************************************************/
#ifndef PROXYPP_SOCKS_PROXY_SESSION_H_
#define PROXYPP_SOCKS_PROXY_SESSION_H_
#include "proxypp/proxy_session.h"
#include "uvcpp.h"
#include "proxypp/socks/socks_req_parser.h"
#include "nul/buffer_pool.hpp"

namespace proxypp {
  /**
   * This class MUST be used with std::shared_ptr
   */
  class SocksProxySession :
    public ProxySession, public std::enable_shared_from_this<SocksProxySession> {
    public:
      SocksProxySession(
        const std::shared_ptr<uvcpp::Tcp> &conn,
        const std::shared_ptr<nul::BufferPool> &bufferPool);
      virtual void start() override;
      virtual void close() override;
      void setUsername(const std::string &username);
      void setPassword(const std::string &password);

    private:
      void replySocksError();
      void connectUpstream();
      void connectUpstream(uvcpp::SockAddr *sockAddr);
      void connectUpstream(const std::string &ip);
      void createUpstreamConnection();
    
    private:
      std::shared_ptr<uvcpp::Tcp> downstreamConn_;
      std::shared_ptr<uvcpp::Tcp> upstreamConn_;
      std::shared_ptr<uvcpp::DNSRequest> dnsRequest_;
      uvcpp::EvDNSResult::DNSResultVector ipAddrs_;
      decltype(ipAddrs_.begin()) ipIt_{ipAddrs_.end()};
      bool upstreamConnected_{false};

      std::shared_ptr<nul::BufferPool> bufferPool_;

      SocksReqParser socks_;
      std::string username_;
      std::string password_;
  };
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_SOCKS_PROXY_SESSION_H_ */
