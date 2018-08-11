/*******************************************************************************
**          File: socks.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:35 PM
**   Description: the class that parses socks messages 
*******************************************************************************/
#ifndef SOCKS_H_
#define SOCKS_H_
#include <string>

namespace sockspp {
  class Socks {
    public:
      enum class Method {
        NO_AUTHENTICATION = 0,
        GSSAPI            = 1,
        USERNAME_PASSWORD = 2
      };

      enum class RequestType {
        CONNECT       = 1,
        BIND          = 2,
        UDP_ASSOCIATE = 3
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

      enum class AddressType {
        UNKNOWN     = 0,
        IPV4        = 1,
        DOMAIN_NAME = 3,
        IPV6        = 4
      };

      enum class State {
        METHOD_IDENTIFICATION         = 0,
        USERNAME_PASSWORD = 1,
        PARSING_REQUEST               = 2,
        NEGOTIATION_COMPLETE          = 3,
        ERROR                         = 4,
      };

      ReplyField parse(const char *buf, std::size_t len);
      void setState(State state);
      State getState() const;
      AddressType getAddressType() const;
      std::string getAddress() const;
      uint16_t getPort() const;

      void setRequireAuthMethod(Method method);
      std::string getParsedUsername() const;
      std::string getParsedPassword() const;

    private:
      ReplyField identifyMethod(const char *buf, std::size_t len);
      ReplyField parseRequest(const char *buf, std::size_t len);
      ReplyField extractUsernamePassword(const char *buf, std::size_t len);
    
    private:
      State state_{State::METHOD_IDENTIFICATION};

      AddressType atyp_{AddressType::UNKNOWN};
      std::string addr_;
      uint16_t port_{0};

      Method requireAuthMethod_{Method::NO_AUTHENTICATION};
      std::string parsedUsername_;
      std::string parsedPassword_;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKS_H_ */
