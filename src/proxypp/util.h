/*******************************************************************************
**          File: util.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-12-17 Tue 12:02 AM
**   Description: some utilities 
*******************************************************************************/
#ifndef PROXYPP_UTIL_H_
#define PROXYPP_UTIL_H_
#include <string>

namespace proxypp {
  class Util {
    public:
      static bool strStartsWith(
        const std::string &s,
        const std::string &prefix,
        std::size_t prefixOffset);
  };
} /* end of namespace: proxypp */

#endif /* end of include guard: UTIL_H_ */
