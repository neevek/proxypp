/*******************************************************************************
**          File: auto_proxy_rule.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-12-12 Thu 11:20 PM
**   Description: a class that represents an auto proxy rule 
*******************************************************************************/
#ifndef AUTO_PROXY_RULE_H_
#define AUTO_PROXY_RULE_H_
#include <string>

namespace proxypp {
  class AutoProxyRule {
    public:
      using MatchFun = std::function<bool(
        const std::string &rule, const std::string &host, uint16_t port)>;

      AutoProxyRule(const std::string &rule, MatchFun &&matchFun) :
        rule_(rule), matches_(matchFun) { }

      bool matches(const std::string &host, uint16_t port) {
        if (matches_(rule_, host, port)) {
          ++matchCount_;
          return true;
        }
        return false;
      }

      bool isRule(const std::string &rule) const {
        return rule_ == rule;
      }

      uint64_t getMatchCount() const {
        return matchCount_;
      }
    
    private:
      std::string rule_;
      MatchFun matches_;
      uint64_t matchCount_{0};
  };
} /* end of namespace: proxypp */

#endif /* end of include guard: AUTO_PROXY_RULE_H_ */
