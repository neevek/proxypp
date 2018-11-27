/*******************************************************************************
**          File: http_proxy_session.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:03 PM
**   Description: object that wraps the tcp connection to the HTTP proxy server
**                and connection to the remote         
*******************************************************************************/
#ifndef SOCKSPP_HTTP_PROXY_SESSION_H_
#define SOCKSPP_HTTP_PROXY_SESSION_H_
#include "sockspp/proxy_session.h"
#include "sockspp/upstream_type.h"
#include "sockspp/proxy_rule_manager.h"
#include "sockspp/socks/socks_client.h"
#include "uvcpp.h"
#include "nul/buffer_pool.hpp"

namespace sockspp {
  /**
   * This class MUST be used with std::shared_ptr
   */
  class HttpProxySession final :
    public ProxySession, public std::enable_shared_from_this<HttpProxySession> {

    public:
      HttpProxySession(
        std::unique_ptr<uvcpp::Tcp> &&conn,
        const std::shared_ptr<nul::BufferPool> &bufferPool);
      virtual void start() override;
      virtual void close() override;

      void setUpstreamServer(
        UpstreamType type, const std::string &ip, uint16_t port);
      void setProxyRuleManager(
        const std::shared_ptr<ProxyRuleManager> &proxyRuleManager);

    private:
      void replyDownstream(const std::string &message);
      void connectUpstreamWithAddr(const std::string &host, uint16_t port);
      void connectUpstreamWithIp(const std::string &ip, uint16_t port);
      void createUpstreamConnection(uint16_t port);

      void onUpstreamConnected(uvcpp::Tcp &conn);

      // targetServerAddr can be IPv4, IPv6 or domain name
      void initiateSocksConnection(
        const std::string &targetServerAddr, uint16_t targetServerPort);
    
    private:
      std::unique_ptr<uvcpp::Tcp> downstreamConn_{nullptr};
      std::shared_ptr<uvcpp::Tcp> upstreamConn_{nullptr};
      std::unique_ptr<uvcpp::DNSRequest> dnsRequest_{nullptr};
      uvcpp::EvDNSResult::DNSResultVector ipAddrs_;
      decltype(ipAddrs_.begin()) ipIt_{ipAddrs_.end()};
      bool upstreamConnected_{false};

      std::shared_ptr<nul::BufferPool> bufferPool_;

      std::string requestData_;

      std::unique_ptr<SocksClient> socksClient_{nullptr};
      UpstreamType upstreamType_{UpstreamType::kUnknown};
      std::string upstreamServerHost_;
      uint16_t upstreamServerPort_{0};
      std::shared_ptr<ProxyRuleManager> proxyRuleManager_{nullptr};
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_HTTP_PROXY_SESSION_H_ */
