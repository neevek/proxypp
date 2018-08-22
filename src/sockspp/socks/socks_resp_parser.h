/*******************************************************************************
**          File: socks_resp_parser.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-25 Wed 06:35 PM
**   Description: the class that parses SOCKS messages 
*******************************************************************************/
#ifndef SOCKSPP_SOCKS_RESP_PARSER_H_
#define SOCKSPP_SOCKS_RESP_PARSER_H_
#include "sockspp/socks/socks.h"
#include <string>

namespace sockspp {
  class SocksRespParser {
    public:
      enum class State {
        METHOD_IDENTIFICATION  = 0,
        REQUEST                = 1,
        USERNAME_PASSWORD_AUTH = 2,
        NEGOTIATION_COMPLETE   = 3
      };

      bool parse(const char *buf, std::size_t len);
      State getState() const;

    private:
      bool parseMethodIndentificationResp(const char *buf, std::size_t len);
      bool parseAuthResp(const char *buf, std::size_t len);
      bool parseRequestResp(const char *buf, std::size_t len);
    
    private:
      State state_{State::METHOD_IDENTIFICATION};
      int chosenMethod_{static_cast<int>(Socks::Method::NO_ACCEPTABLE_METHODS)};
      std::string boundAddr_;
      uint16_t boundPort_{0};
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_SOCKS_RESP_PARSER_H_ */
