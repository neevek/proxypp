/*******************************************************************************
**          File: socks_client.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-20 Mon 05:02 PM
**   Description: see the header fiel 
*******************************************************************************/
#include "socks_client.h"
#include <array>

namespace sockspp {

  SocksClient::SocksClient(
    const std::shared_ptr<uvcpp::Loop> &loop,
    std::shared_ptr<nul::BufferPool> bufferPool) :
    conn_(uvcpp::Tcp::createUnique(loop)),
    bufferPool_(bufferPool) {
  }

  bool SocksClient::connect(const std::string &serverHost, uint16_t serverPort) {
    return conn_->connect(serverHost, serverPort);
  }

  void SocksClient::startHandshake(
    const std::string &targetHost, uint16_t targetPort) {
    if (targetHost.empty() || targetPort == 0) {
      conn_->publish(EvSocksHandshake{false});
      return;
    }

    conn_->on<uvcpp::EvRead>(
      [this, targetHost, targetPort](const auto &e, auto &client){

      auto state = socks_.getState();
      if (state == SocksRespParser::State::NEGOTIATION_COMPLETE) {
        conn_->publish(EvSocksRead{e.buf, e.nread});

      } else {
        if (!socks_.parse(e.buf, e.nread)) {
          client.close();
          conn_->publish(EvSocksHandshake{false});
          return;
        }

        if (state == SocksRespParser::State::REQUEST) {
          this->sendSocksRequest(targetHost, targetPort);

        } else if (state == SocksRespParser::State::USERNAME_PASSWORD_AUTH) {
          auto authBuf = std::string{"\1"};
          authBuf.append(1, static_cast<char>(username_.length()));
          authBuf.append(username_);
          authBuf.append(1, static_cast<char>(password_.length()));
          authBuf.append(password_);
          client.writeAsync(
            bufferPool_->assembleDataBuffer(authBuf.c_str(), authBuf.length()));

        } else if (state == SocksRespParser::State::NEGOTIATION_COMPLETE) {
          conn_->publish(EvSocksHandshake{true});
        }
      }
    });
    conn_->readStart();

    sendInitialRequest();
  }

  void SocksClient::sendInitialRequest() {
    int methods = 1;  // NO AUTHENTICATION REQUIRED
    if (!username_.empty() || !password_.empty()) {
      ++methods;
    }

    std::unique_ptr<nul::Buffer> reqBuf = nullptr;
    if (methods == 1) {
      const char req[] = "\5\1\0";
      reqBuf = bufferPool_->assembleDataBuffer(req, sizeof(req));
    } else {
      const char req[] = "\5\2\0\2";  // \2 = username/password auth
      reqBuf = bufferPool_->assembleDataBuffer(req, sizeof(req));
    }
    conn_->writeAsync(std::move(reqBuf));
  }

  void SocksClient::sendSocksRequest(
    const std::string &targetHost, uint16_t targetPort) {
    uvcpp::SockAddrStorage sas;

    Socks::AddressType atyp = Socks::AddressType::UNKNOWN;
    auto lastCh = targetHost[targetHost.length() - 1];
    if (lastCh >= '0' && lastCh <= '9') { // treat it as IPv4
      if (uv_ip4_addr(
          targetHost.c_str(),
          targetPort,
          reinterpret_cast<uvcpp::SockAddr4 *>(&sas)) == 0) {
        atyp = Socks::AddressType::IPV4;
      }
    }

    if (atyp == Socks::AddressType::UNKNOWN &&
        targetHost.find("::") != std::string::npos) {
      if (uv_ip6_addr(
          targetHost.c_str(),
          targetPort,
          reinterpret_cast<uvcpp::SockAddr6 *>(&sas)) == 0) {
        atyp = Socks::AddressType::IPV6;
      }
    }

    if (atyp == Socks::AddressType::UNKNOWN) {
      atyp = Socks::AddressType::DOMAIN_NAME;
    }

    auto reqBuf = std::string{"\5\1\0"};  // send CONNECT request
    switch(atyp) {
      case Socks::AddressType::IPV4: {
        auto sa4 = reinterpret_cast<uvcpp::SockAddr4 *>(&sas);
        reqBuf.append(1, '\1');
        reqBuf.append(reinterpret_cast<char *>(&sa4->sin_addr), 4);
        break;
      }
      case Socks::AddressType::DOMAIN_NAME: {
        reqBuf.append(1, '\3');
        reqBuf.append(1, static_cast<char>(targetHost.length()));
        reqBuf.append(targetHost);
        break;
      }
      case Socks::AddressType::IPV6: {
        auto sa6 = reinterpret_cast<uvcpp::SockAddr6 *>(&sas);
        reqBuf.append(1, '\4');
        reqBuf.append(reinterpret_cast<char *>(&sa6->sin6_addr), 16);
        break;
      }
      default:
        break;
    }
    auto port = htons(targetPort);
    reqBuf.append(reinterpret_cast<char *>(&port), 2);

    conn_->writeAsync(
      bufferPool_->assembleDataBuffer(reqBuf.c_str(), reqBuf.length()));
  }

} /* end of namspace: sockapp */
