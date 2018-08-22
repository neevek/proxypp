/*******************************************************************************
**          File: socks_req_parser.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:35 PM
**   Description: the class that parses SOCKS messages 
*******************************************************************************/
#ifndef SOCKSPP_SOCKS_REQ_PARSER_H_
#define SOCKSPP_SOCKS_REQ_PARSER_H_
#include <string>
#include "sockspp/socks/socks.h"

namespace sockspp {
  class SocksReqParser {
    public:
      enum class State {
        METHOD_IDENTIFICATION         = 0,
        USERNAME_PASSWORD_AUTH        = 1,
        PARSING_REQUEST               = 2,
        NEGOTIATION_COMPLETE          = 3,
        ERROR_OCCURRED                = 4,
      };

      enum class ReplyField {
        SUCCEEDED                    = 0,
        GENERAL_SOCKS_SERVER_FAILURE = 1,
        CONNECTION_NOT_ALLOWED       = 2,
        NETWORK_UNREACHABLE          = 3,
        HOST_UNREACHABLE             = 4,
        CONNECTION_REFUSED           = 5,
        TTL_EXPIRED                  = 6,
        COMMAND_NOT_SUPPORTED        = 7,
        ADDRESS_TYPE_NOT_SUPPORTED   = 8
      };

      ReplyField parse(const char *buf, std::size_t len);
      void setState(State state);
      State getState() const;
      Socks::AddressType getAddressType() const;
      std::string getAddress() const;
      uint16_t getPort() const;

      void setRequireAuthMethod(Socks::Method method);
      std::string getParsedUsername() const;
      std::string getParsedPassword() const;

    private:
      ReplyField identifyMethod(const char *buf, std::size_t len);
      ReplyField parseRequest(const char *buf, std::size_t len);
      ReplyField extractUsernamePassword(const char *buf, std::size_t len);
    
    private:
      State state_{State::METHOD_IDENTIFICATION};

      Socks::AddressType atyp_{Socks::AddressType::UNKNOWN};
      std::string addr_;
      uint16_t port_{0};

      Socks::Method requireAuthMethod_{Socks::Method::NO_AUTHENTICATION};
      std::string parsedUsername_;
      std::string parsedPassword_;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_SOCKS_REQ_PARSER_H_ */
