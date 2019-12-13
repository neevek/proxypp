/*******************************************************************************
**          File: auto_proxy_manager.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-12-13 Fri 09:19 PM
**   Description: 
*******************************************************************************/
#include "auto_proxy_manager.h"
#include <regex>
#include <algorithm>
#include <functional>
#include <fstream>
#include "nul/log.hpp"

namespace {
  constexpr int SORT_RULES_THRESHOLD = 3;
  const auto RULE_COMPARATOR = [](
    const proxypp::AutoProxyRule &lhs, const proxypp::AutoProxyRule &rhs) {
    return lhs.getMatchCount() > rhs.getMatchCount();
  };
}

namespace proxypp {
  bool AutoProxyManager::addRule(const std::string &ruleStr) {
    auto rule = parse(ruleStr);
    if (rule) {
      matchRules_.emplace_back(ruleStr, std::move(rule));
      return true;
    }

    rule = parseExceptionRule(ruleStr);
    if (rule) {
      exceptionRules_.emplace_back(ruleStr, std::move(rule));
      return true;
    }

    return false;
  }

  bool AutoProxyManager::removeRule(const std::string &rule) {
    if (!removeRuleFrom(matchRules_, rule)) {
      return removeRuleFrom(exceptionRules_, rule);
    }
    return false;
  }

  std::size_t AutoProxyManager::parseFileAsRules(const std::string &file) {
    std::ifstream ruleFileStream(file, std::ios::binary);
    if (!ruleFileStream.is_open()) {
      LOG_W("proxy rule file not exists: %s", file.c_str());
      return 0;
    }

    std::size_t count = 0;
    std::string line;
    while (std::getline(ruleFileStream, line)) {
      if (addRule(line)) {
        ++count;
      }
    }
    return count;
  }

  void AutoProxyManager::clearAll() {
    matchRules_.clear();
    exceptionRules_.clear();
  }

  bool AutoProxyManager::matches(const std::string &host, uint16_t port) {
    auto size = matchRules_.size();
    auto &rules = matchRules_;
    for (int i = 0; i < size; ++i) {
      auto &rule = rules[i];
      if (rule.matches(host, port)) {
        if (i > SORT_RULES_THRESHOLD) {
          ++matchCount_;
          if (matchCount_ % SORT_RULES_THRESHOLD == 0) {
            std::sort(rules.begin(), rules.end(), RULE_COMPARATOR);
          }
        }

        for (auto &exceptRule : exceptionRules_) {
          if (exceptRule.matches(host, port)) {
            return false;
          }
        }
        return true;
      }
    }
    return false;
  }

  AutoProxyRule::MatchFun AutoProxyManager::parse(const std::string &rule) {
    if (rule.empty()) {
      return nullptr;
    }

    auto size = rule.size();
    auto ch = std::tolower(rule[0]);
    if (size > 2 && ch == '/' && rule[size - 1] == '/') {
      // it's a regex
      return [r = std::regex(rule.substr(1, size - 1))] (
        const std::string &rule, const std::string &host, uint16_t port) {
        return std::regex_match(host, r);
      };
    }

    if (ch != '|' && ch != '.' &&
        (ch < '0' || ch > '9') && (ch < 'a' || ch > 'z')) {
      return nullptr;
    }

    // matches against the entire rule
    if (strStartsWith(rule, "|https://", 0)) {
      return [] (
        const std::string &rule, const std::string &host, uint16_t port) {
        return port == 443 && strStartsWith(host, rule, 9);
      };
    }

    if (strStartsWith(rule, "|http://", 0)) {
      return [] (
        const std::string &rule, const std::string &host, uint16_t port) {
        return port != 443 && strStartsWith(host, rule, 8);
      };
    }

    // matches against domain
    std::string ruleStartsWithDot;
    if (strStartsWith(rule, "||.", 0)) {
      ruleStartsWithDot = rule.substr(2);

    } else if (strStartsWith(rule, "||", 0)) {
      ruleStartsWithDot = "." + rule.substr(2);

    } else if (rule[0] != '.') {
      ruleStartsWithDot = "." + rule;

    } else {
      ruleStartsWithDot = rule;
    }
    // treat the rule as having wildcards both at the start and the end
    return [ruleStartsWithDot] (
      const std::string &, const std::string &host, uint16_t port) {
      return host.find(ruleStartsWithDot) != std::string::npos ||
        (strStartsWith(host, ruleStartsWithDot, 1));
    };
  }

  AutoProxyRule::MatchFun AutoProxyManager::parseExceptionRule(
    const std::string &rule) {
    if (strStartsWith(rule, "@@|https://", 0)) {
      return [] (
        const std::string &rule, const std::string &host, uint16_t port) {
        return port == 443 && strStartsWith(host, rule, 11);
      };
    }

    if (strStartsWith(rule, "@@|http://", 0)) {
      return [] (
        const std::string &rule, const std::string &host, uint16_t port) {
        return port != 443 && strStartsWith(host, rule, 10);
      };
    }

    if (strStartsWith(rule, "@@||", 0)) {
      std::string ruleStartsWithDot;
      if (rule[0] != '.') {
        ruleStartsWithDot = "." + rule.substr(4);
      } else {
        ruleStartsWithDot = rule;
      }

      return [ruleStartsWithDot] (
        const std::string &, const std::string &host, uint16_t port) {
        return host.find(ruleStartsWithDot) != std::string::npos ||
          (strStartsWith(host, ruleStartsWithDot, 1));
      };
    }

    return nullptr;
  }

  bool AutoProxyManager::strStartsWith(
    const std::string &s,
    const std::string &prefix,
    std::size_t prefixOffset) {
    if (s.size() < (prefix.size() - prefixOffset) ||
        prefixOffset >= prefix.size()) {
      return false;
    }
    for (int i = 0, j = prefixOffset; j < prefix.size(); ++i, ++j) {
      if (s[i] != prefix[j]) {
        return false;
      }
    }
    return true;
  }

  bool AutoProxyManager::removeRuleFrom(
    std::vector<AutoProxyRule> &vec, const std::string &rule) {
    auto it = vec.begin();
    while (it != vec.end()) {
      if (it->isRule(rule)) {
        vec.erase(it);
        return true;
      }
    }
    return false;
  }
} /* end of namespace: proxypp */
