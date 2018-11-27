/*******************************************************************************
**          File: socks_req_parser.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-26 Thu 02:22 PM
**   Description: see the header file 
*******************************************************************************/
#include "proxypp/socks/socks_req_parser.h"
#include "nul/log.hpp"

namespace {
  #define SOCKS_ERROR(replyField, fmt, ...) \
    state_ = State::ERROR_OCCURRED; \
    LOG_E(fmt, ##__VA_ARGS__); \
    return replyField 
}

namespace proxypp {

  SocksReqParser::ReplyField SocksReqParser::parse(const char *buf, std::size_t len) {
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

      case State::USERNAME_PASSWORD_AUTH:
        return extractUsernamePassword(buf, len);

      case State::PARSING_REQUEST:
        return parseRequest(buf, len);

      default:
        return ReplyField::GENERAL_SOCKS_SERVER_FAILURE;
    }
    return ReplyField::SUCCEEDED;
  }

  SocksReqParser::ReplyField SocksReqParser::identifyMethod(const char *buf, std::size_t len) {
    if (*buf != Socks::VERSION) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "Bad SOCKS version: %d", *buf);
    }
    std::size_t count = *(buf + 1);
    if (count != len - 2) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "Bad SOCKS method identification message");
    }
    buf += 2;
    for (std::size_t i = 0; i < count; ++i) {
      auto authMethod = static_cast<Socks::Method>(*(buf + i));
      if (authMethod == requireAuthMethod_) {
        state_ = authMethod == Socks::Method::NO_AUTHENTICATION ?
          State::PARSING_REQUEST :
          State::USERNAME_PASSWORD_AUTH;

        return ReplyField::SUCCEEDED;
      }
    }

    if (requireAuthMethod_ == Socks::Method::USERNAME_PASSWORD) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "Username/Password authenticat is required");
    } else {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "NO_AUTHENTICATION is needed");
    }
  }

  SocksReqParser::ReplyField SocksReqParser::extractUsernamePassword(
    const char *buf, std::size_t len) {
    if (len < 3) {
      // simply return SUCCEEDED but do not parse username and password,
      // so later authentication will certainly fail
      return ReplyField::SUCCEEDED;
    }
    //auto subVersion = *buf;
    buf += 1;
    len -= 1;

    std::size_t usernameLen = *buf;
    buf += 1;
    len -= 1;
    if (usernameLen >= len) {
      // invalid username length, return SUCCEEDED but do not parse username,
      // later authentication will fail
      return ReplyField::SUCCEEDED;
    }
    parsedUsername_.assign(buf, usernameLen);
    buf += usernameLen;
    len -= usernameLen;

    std::size_t passwordLen = *buf;
    buf += 1;
    len -= 1;
    if (passwordLen != len) {
      // invalid password length, return SUCCEEDED but do not parse password,
      // later authentication will fail
      return ReplyField::SUCCEEDED;
    }

    parsedPassword_.assign(buf, passwordLen);
    return ReplyField::SUCCEEDED;
  }

  SocksReqParser::ReplyField SocksReqParser::parseRequest(const char *buf, std::size_t len) {
    if (len < 4) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "SOCKS message too short");
    }
    auto version = *buf;
    auto cmd     = *(buf + 1);
    //auto rsv     = *(buf + 2);
    auto atyp    = *(buf + 3);

    if (version != Socks::VERSION) {
      SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                  "Invalid SOCKS version: %d", version);
    }

    if (cmd != static_cast<int>(Socks::RequestType::CONNECT)) {
      SOCKS_ERROR(ReplyField::COMMAND_NOT_SUPPORTED,
                  "unsupported command: %d", cmd);
    }

    buf += 4;
    len -= 4;

    switch(static_cast<Socks::AddressType>(atyp)) {
      case Socks::AddressType::IPV4:
        if (len != 4 + 2) {
          SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                      "Incorrect IPV4 address length: %zu", len);
        }
        addr_.assign(buf, 4);
        buf += 4;
        break;
      case Socks::AddressType::IPV6:
        if (len != 16 + 2) {
          SOCKS_ERROR(ReplyField::GENERAL_SOCKS_SERVER_FAILURE,
                      "Incorrect IPV6 address length: %zu", len);
        }
        addr_.assign(buf, 16);
        buf += 16;
        break;
      case Socks::AddressType::DOMAIN_NAME: {
        std::size_t addrLen = *buf;
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

    atyp_ = static_cast<Socks::AddressType>(atyp);
    memcpy(&port_, buf, 2);
    state_ = State::NEGOTIATION_COMPLETE;
    return ReplyField::SUCCEEDED;
  }

  void SocksReqParser::setState(SocksReqParser::State state) {
    state_ = state;
  }

  SocksReqParser::State SocksReqParser::getState() const {
    return state_;
  }

  Socks::AddressType SocksReqParser::getAddressType() const {
    return atyp_;
  }

  std::string SocksReqParser::getAddress() const {
    return addr_;
  }

  uint16_t SocksReqParser::getPort() const {
    return port_;
  }

  void SocksReqParser::setRequireAuthMethod(Socks::Method method) {
    requireAuthMethod_ = method;
  }

  std::string SocksReqParser::getParsedUsername() const {
    return parsedUsername_;
  }

  std::string SocksReqParser::getParsedPassword() const {
    return parsedPassword_;
  }

} /* end of namspace: proxypp */
