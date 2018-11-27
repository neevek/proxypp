/*******************************************************************************
**          File: upstream_type.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-11-27 Tue 09:52 AM
**   Description: socks5 or http
*******************************************************************************/
#ifndef PROXYPP_UPSTREAM_TYPE_H_
#define PROXYPP_UPSTREAM_TYPE_H_

namespace proxypp {
  enum class UpstreamType {
    kUnknown,
    kSOCKS5,
    kHTTP
  };
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_UPSTREAM_TYPE_H_ */
