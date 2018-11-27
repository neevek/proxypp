/*******************************************************************************
**          File: socks_client.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-20 Mon 04:51 PM
**   Description: SOCKS client implementation
*******************************************************************************/
#ifndef PROXYPP_SOCKS_CLIENT_H_
#define PROXYPP_SOCKS_CLIENT_H_
#include "uvcpp.h"
#include "proxypp/socks/socks_resp_parser.h"
#include "nul/buffer_pool.hpp"

namespace proxypp {

  struct EvSocksRead : public uvcpp::EvRead {
    EvSocksRead(const char *buf, ssize_t nread) : uvcpp::EvRead(buf, nread) { }
  };

  struct EvSocksHandshake : public uvcpp::Event {
    EvSocksHandshake(bool succeeded) : succeeded(succeeded) { }
    bool succeeded{false};
  };

  class SocksClient final {
    public:
      SocksClient(
        const std::shared_ptr<uvcpp::Loop> &loop,
        std::shared_ptr<nul::BufferPool> bufferPool);
      bool connect(const std::string &serverHost, uint16_t serverPort);
      void startHandshake(const std::string &targetHost, uint16_t targetPort);

      // disable uvcpp::EvRead for TunnelConn, use EvSocksRead instead
      template<typename E, typename =
        std::enable_if_t<!std::is_same<E, uvcpp::EvRead>::value, E>>
      void on(uvcpp::EventCallback<E, uvcpp::Tcp> &&callback) {
        conn_->on<E>(
          std::forward<uvcpp::EventCallback<E, uvcpp::Tcp>>(callback));
      }

      // disable uvcpp::EvRead for TunnelConn, use EvSocksRead instead
      template<typename E, typename =
        std::enable_if_t<!std::is_same<E, uvcpp::EvRead>::value, E>>
      void once(uvcpp::EventCallback<E, uvcpp::Tcp> &&callback) {
        conn_->once<E>(
          std::forward<uvcpp::EventCallback<E, uvcpp::Tcp>>(callback));
      }

      void writeAsync(std::unique_ptr<nul::Buffer> &&buffer);
      void close();
      void setUsername(const std::string &username);
      void setPassword(const std::string &password);

    private:
      void sendInitialRequest();
      void sendSocksRequest(const std::string &targetHost, uint16_t targetPort);
    
    private:
      std::unique_ptr<uvcpp::Tcp> conn_{nullptr};
      std::shared_ptr<nul::BufferPool> bufferPool_{nullptr};
      std::string username_;
      std::string password_;

      SocksRespParser socks_;
  };
  
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_SOCKS_CLIENT_H_ */
