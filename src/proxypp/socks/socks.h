/*******************************************************************************
**          File: socks.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-20 Mon 05:32 PM
**   Description: SOCKS related constants
*******************************************************************************/
#ifndef PROXYPP_SOCKS_H_
#define PROXYPP_SOCKS_H_

namespace proxypp {
  struct Socks {
    constexpr static auto VERSION = 5;

    enum class Method {
      NO_AUTHENTICATION     = 0,
      GSSAPI                = 1,
      USERNAME_PASSWORD     = 2,
      NO_ACCEPTABLE_METHODS = 0xff
    };

    enum class RequestType {
      CONNECT       = 1,
      BIND          = 2,
      UDP_ASSOCIATE = 3
    };

    enum class AddressType {
      UNKNOWN     = 0,
      IPV4        = 1,
      DOMAIN_NAME = 3,
      IPV6        = 4
    };
  };
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_SOCKS_H_ */
