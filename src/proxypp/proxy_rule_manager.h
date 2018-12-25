/*******************************************************************************
**          File: proxy_rule_manager.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-11-26 Mon 11:34 AM
**   Description: contains regex rules to determine whether to forward
**                the traffic to upstream or connect the requested server
**                directly
*******************************************************************************/
#ifndef PROXYPP_PROXY_RULE_MANAGER_H_
#define PROXYPP_PROXY_RULE_MANAGER_H_
#include <vector>
#include <map>
#include <regex>

namespace proxypp {
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
      void addProxyRule(const std::string &regexStr);
      void removeProxyRule(const std::string &regexStr);

      void addIgnoreRulesWithFile(const std::string &ingoreRulesFile);
      void addIgnoreRulesWithString(const std::string &ingoreRulesString);
      void addIgnoreRule(const std::string &regexStr);
      void removeIgnoreRule(const std::string &regexStr);

      bool shouldForwardToUpstream(const std::string &host) const;
      Mode getMode() const;
      void setProxyRulesMode(Mode mode);
    
    private:
      std::map<std::string, std::regex> regexProxyRules_;
      std::map<std::string, std::regex> regexIgnoreRules_;
      Mode mode_;
  };
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_PROXY_RULE_MANAGER_H_ */
