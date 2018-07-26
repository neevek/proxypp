/*******************************************************************************
**          File: client.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:03 PM
**   Description: client wraps the tcp connection to the socks server
*******************************************************************************/
#ifndef CLIENT_H_
#define CLIENT_H_
#include "uvcpp.h"
#include "socks.h"
#include "buffer_pool.h"

namespace sockspp {
  class Client {
    public:
      using Id = uint32_t;

      Client(Id id, std::unique_ptr<uvcpp::Tcp> &&conn,
             const std::shared_ptr<BufferPool> &bufferPool);

      void start();
      Id getId() const;

    private:
      void replySocksError();
      void connectUpstream();
      void connectUpstream(uvcpp::SockAddr *sockAddr);
      void connectUpstream(const std::string &ip);
      void createUpstreamConnection();
    
    private:
      Id id_{0};
      std::unique_ptr<uvcpp::Tcp> downstreamConn_{nullptr};
      std::unique_ptr<uvcpp::Tcp> upstreamConn_{nullptr};
      std::unique_ptr<uvcpp::DNSRequest> dnsRequest_{nullptr};
      uvcpp::EvDNSResult::DNSResultVector ipAddrs_;
      decltype(ipAddrs_.begin()) ipIt_{ipAddrs_.end()};
      bool upstreamConnected_{false};

      std::shared_ptr<BufferPool> bufferPool_;

      Socks socks_;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: CLIENT_H_ */
