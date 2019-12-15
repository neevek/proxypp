/*******************************************************************************
**          File: auto_proxy_manager.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-12-13 Fri 08:50 PM
**   Description:
*******************************************************************************/
#ifndef AUTO_PROXY_MANAGER_H_
#define AUTO_PROXY_MANAGER_H_
#include <string>
#include <vector>
#include "auto_proxy_rule.h"

namespace proxypp {

  class AutoProxyManager final {
    public:
      bool addRule(const std::string &rule);
      bool removeRule(const std::string &rule);
      std::size_t parseFileAsRules(const std::string &file);
      bool matches(const std::string &host, uint16_t port);
      void clearAll();

    private:
      static AutoProxyRule::MatchFun parse(const std::string &rule);
      static AutoProxyRule::MatchFun parseExceptionRule(const std::string &rule);

      static bool removeRuleFrom(
        std::vector<AutoProxyRule> &vec, const std::string &rule);

    private:
      std::vector<AutoProxyRule> matchRules_;
      std::vector<AutoProxyRule> exceptionRules_;
      std::size_t matchCount_{0};
  };

} /* end of namespace: proxypp */

#endif /* end of include guard: AUTO_PROXY_MANAGER_H_ */
