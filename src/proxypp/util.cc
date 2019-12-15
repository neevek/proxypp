/*******************************************************************************
**          File: util.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-12-17 Tue 12:13 AM
**   Description: see the header 
*******************************************************************************/
#include "util.h"

namespace proxypp {
  bool Util::strStartsWith(
    const std::string &s,
    const std::string &prefix,
    std::size_t prefixOffset) {

    if (s.size() < (prefix.size() - prefixOffset) ||
        prefixOffset >= prefix.size()) {
      return false;
    }
    for (std::size_t i = 0, j = prefixOffset; j < prefix.size(); ++i, ++j) {
      if (s[i] != prefix[j]) {
        return false;
      }
    }
    return true;
  }
} /* end of namespace: proxypp */
