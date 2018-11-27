/*******************************************************************************
**          File: upstream_type.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-11-27 Tue 09:52 AM
**   Description: socks5 or http
*******************************************************************************/
#ifndef SOCKSPP_UPSTREAM_TYPE_H_
#define SOCKSPP_UPSTREAM_TYPE_H_

namespace sockspp {
  enum class UpstreamType {
    kUnknown,
    kSOCKS5,
    kHTTP
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_UPSTREAM_TYPE_H_ */
