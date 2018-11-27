/*******************************************************************************
**          File: proxy_rule_manager.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-11-26 Mon 11:42 AM
**   Description: see the header file 
*******************************************************************************/
#include "proxy_rule_manager.h"
#include "nul/log.hpp"
#include <fstream>
#include <sstream>

namespace sockspp {

  ProxyRuleManager::ProxyRuleManager(ProxyRuleManager::Mode mode) :
    mode_(mode) {
  }

  void ProxyRuleManager::addProxyRulesWithFile(
    const std::string &proxyRulesFile) {
    auto f = std::ifstream(proxyRulesFile, std::ios::in);

    auto count = 0;
    std::string line;
    while (std::getline(f, line)) {
      if (!line.empty()) {
        addRegexRule(line);
        ++count;
      }
    }
    f.close();

    LOG_I("Loaded %d proxy rules from file: %s",
          count, proxyRulesFile.c_str());
  }

  void ProxyRuleManager::addProxyRulesWithString(
    const std::string &proxyRulesString) {
    auto s = std::istringstream(proxyRulesString);

    auto count = 0;
    std::string line;
    while (std::getline(s, line)) {
      if (!line.empty()) {
        addRegexRule(line);
        ++count;
      }
    }
    LOG_I("Loaded %d proxy rules from string", count);
  }

  void ProxyRuleManager::addRegexRule(const std::string &regexStr) {
    regexRules_.emplace_back(regexStr);
  }

  bool ProxyRuleManager::shouldForwardToUpstream(const std::string &host) const {
    auto isWhiteListMode = mode_ == ProxyRuleManager::Mode::kWhiteList;
    for (auto &r : regexRules_) {
      if (std::regex_match(host, r)) {
        return isWhiteListMode;
      }
    }
    return !isWhiteListMode;
  }

  ProxyRuleManager::Mode ProxyRuleManager::getMode() const {
    return mode_;
  }

  void ProxyRuleManager::setProxyRulesMode(ProxyRuleManager::Mode mode) {
    mode_ = mode;
  }
  
} /* end of namspace: sockspp */
