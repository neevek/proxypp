/*******************************************************************************
**          File: socks.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-26 Thu 02:22 PM
**   Description: see the header file 
*******************************************************************************/
#include "socks.h"
#include "log/log.h"

namespace {
  const static auto VERSION = 0x05;
  #define SOCKS_ERROR(replyField, fmt, ...) \
    state_ = State::ERROR; \
    LOG_E(fmt, ##__VA_ARGS__); \
    return replyField 
}

namespace sockspp {

  Socks::ReplyField Socks::parse(const char *buf, std::size_t len) {
    if (state_ > State::PARSING_REQUEST) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "Invalid state: %d", static_cast<int>(state_));
    }
    if (len < 3) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "SOCKS message too short");
    }

    switch(state_) {
      case State::METHOD_IDENTIFICATION:
        return identifyMethod(buf, len);

      case State::AUTHENTICATION_NEGOTIATION:
        // will not reach here
        break;

      case State::PARSING_REQUEST:
        return parseRequest(buf, len);

      default:
        return ReplyField::GENERAL_SOCKS_SERVER_FAILURE;
    }
    return ReplyField::SUCCEEDED;
  }

  Socks::ReplyField Socks::identifyMethod(const char *buf, std::size_t len) {
    if (*buf != VERSION) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "Bad SOCKS version: %d", *buf);
    }
    auto count = *(buf + 1);
    if (count != len - 2) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "Bad SOCKS method identification message");
    }
    buf += 2;
    for (int i = 0; i < count; ++i) {
      if (*(buf + i) == static_cast<int>(Method::NO_AUTHENTICATION)) {
        state_ = State::PARSING_REQUEST;
        return ReplyField::SUCCEEDED;
      }
    }
    SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                "sockspp only supports NO_AUTHENTICATION");
  }

  Socks::ReplyField Socks::parseRequest(const char *buf, std::size_t len) {
    if (len < 4) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "SOCKS message too short");
    }
    auto version = *buf;
    auto cmd     = *(buf + 1);
    auto rsv     = *(buf + 2);
    auto atyp    = *(buf + 3);

    if (version != VERSION) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "SOCKS message too short");
    }

    if (cmd != static_cast<int>(RequestType::CONNECT)) {
      SOCKS_ERROR(ReplyField::COMMAND_NOT_SUPPORTED,
                  "unsupported command: %d", cmd);
    }

    buf += 4;
    len -= 4;

    switch(static_cast<AddressType>(atyp)) {
      case AddressType::IPV4:
        if (len != 4 + 2) {
          SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                      "Incorrect IPV4 address length: %zu", len);
        }
        addr_.assign(buf, 4);
        buf += 4;
        break;
      case AddressType::IPV6:
        if (len != 16 + 2) {
          SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                      "Incorrect IPV6 address length: %zu", len);
        }
        addr_.assign(buf, 16);
        buf += 16;
        break;
      case AddressType::DOMAIN_NAME: {
        auto addrLen = *buf;
        addr_.assign(buf + 1, addrLen);
        if (len != 1 + addrLen + 2) {
          SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                      "Incorrect domain address length: %zu", len);
        }
        buf += (1 + addrLen);
        break;
      }
      default:
        SOCKS_ERROR(ReplyField::ADDRESS_TYPE_NOT_SUPPORTED,
                    "unknown atyp: %d", atyp);
    }

    atyp_ = static_cast<AddressType>(atyp);
    memcpy(&port_, buf, 2);
    state_ = State::NEGOTIATION_COMPLETE;
    return ReplyField::SUCCEEDED;
  }

  Socks::State Socks::getState() const {
    return state_;
  }

  Socks::AddressType Socks::getAddressType() const {
    return atyp_;
  }

  std::string Socks::getAddress() const {
    return addr_;
  }

  uint16_t Socks::getPort() const {
    return port_;
  }

} /* end of namspace: sockspp */
