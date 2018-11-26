/*******************************************************************************
**          File: proxy_rule_manager.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-11-26 Mon 11:34 AM
**   Description: contains regex rules to determine whether to forward
**                the traffic to upstream or connect the requested server
**                directly
*******************************************************************************/
#ifndef SOCKSPP_PROXY_RULE_MANAGER_H_
#define SOCKSPP_PROXY_RULE_MANAGER_H_
#include <vector>
#include <regex>

namespace sockspp {
  class ProxyRuleManager {
    public:
      enum class Mode {
        kWhiteList,
        kBlackList,
      };

    public:
      ProxyRuleManager(Mode mode);

      void addProxyRulesWithFile(const std::string &proxyRulesFile);
      void addProxyRulesWithString(const std::string &proxyRulesString);
      void addRegexRule(const std::string &regexStr);

      bool shouldForwardToUpstream(const std::string &host) const;
      Mode getMode() const;
      void setProxyRulesMode(Mode mode);
    
    private:
      std::vector<std::regex> regexRules_;
      Mode mode_;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_PROXY_RULE_MANAGER_H_ */
