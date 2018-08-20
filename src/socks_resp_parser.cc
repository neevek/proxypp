/*******************************************************************************
**          File: socks_resp_parser.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-20 Mon 05:48 PM
**   Description: see the header file 
*******************************************************************************/
#include "socks_resp_parser.h"
#include "nul/log.hpp"
#include "uvcpp.h"

namespace sockspp {

  bool SocksRespParser::parse(const char *buf, std::size_t len) {
    if (state_ == State::METHOD_IDENTIFICATION) {
      return parseMethodIndentificationResp(buf, len);

    } else if (state_ == State::REQUEST) {
      return parseRequestResp(buf, len);
    }

    return parseAuthResp(buf, len);
  }

  bool SocksRespParser::parseMethodIndentificationResp(
    const char *buf, std::size_t len) {
    if (len < 2 || buf[0] != '\5') {
      LOG_E("Invalid socks response: length=%zu, version: 0x%d",
            len, len > 0 ? buf[0] : 0);
      return false;
    }

    chosenMethod_ = buf[1];
    if (chosenMethod_ == static_cast<int>(Socks::Method::NO_AUTHENTICATION)) {
      state_ = State::REQUEST;
      return true;
    } else if (chosenMethod_ == static_cast<int>(Socks::Method::USERNAME_PASSWORD)) {
      state_ = State::USERNAME_PASSWORD_AUTH;
      return true;
    }

    LOG_E("Unsupported method: %d", chosenMethod_);
    return false;
  }

  bool SocksRespParser::parseAuthResp(const char *buf, std::size_t len) {
    if (len != 2) {
      LOG_E("username/password auth failed, len=%zu", len);
      return false;
    }
    if (buf[0] != 0x1) {
      LOG_E("Invalid auth response version: %d", buf[0]);
      return false;
    }
    if (buf[1] != 0x0) {
      LOG_E("username/password auth failed: %d", buf[1]);
      return false;
    }

    state_ = State::REQUEST;
    return true;
  }

  bool SocksRespParser::parseRequestResp(const char *buf, std::size_t len) {
    if (len < 4) {
      LOG_E("SOCKS response too short: %zu", len);
      return false;
    }

    auto version = *buf;
    auto rep     = *(buf + 1);
    //auto rsv     = *(buf + 2);
    auto atyp    = *(buf + 3);

    if (version != Socks::VERSION) {
      LOG_E("Invalid SOCKS version: %d", version);
      return false;
    }

    if (rep != 0) {
      LOG_E("Invalid SOCKS reply: %d", rep);
      return false;
    }

    buf += 4;
    len -= 4;

    switch(static_cast<Socks::AddressType>(atyp)) {
      case Socks::AddressType::IPV4:
        if (len != 4 + 2) {
          LOG_W("Incorrect IPV4 address length: %zu", len);
          return false;
        }
        boundAddr_.assign(buf, 4);
        buf += 4;
        break;
      case Socks::AddressType::IPV6:
        if (len != 16 + 2) {
          LOG_W("Incorrect IPV6 address length: %zu", len);
          return false;
        }
        boundAddr_.assign(buf, 16);
        buf += 16;
        break;
      case Socks::AddressType::DOMAIN_NAME: {
        std::size_t addrLen = *buf;
        boundAddr_.assign(buf + 1, addrLen);
        if (len != 1 + addrLen + 2) {
          LOG_W("Incorrect domain address length: %zu", len);
          return false;
        }
        buf += (1 + addrLen);
        break;
      }
      default:
        LOG_W("Incorrect atyp: %d", atyp);
        return false;
    }

    memcpy(&boundPort_, buf, 2);
    boundPort_ = ntohs(boundPort_);
    state_ = State::NEGOTIATION_COMPLETE;
    return true;
  }

  SocksRespParser::State SocksRespParser::getState() const {
    return state_;
  }

} /* end of namspace: sockspp */
