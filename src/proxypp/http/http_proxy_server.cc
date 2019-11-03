/*******************************************************************************
**          File: http_proxy_server.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 10:12 AM
**   Description: see the header file 
*******************************************************************************/
#include "proxypp/http/http_proxy_server.h"
#include "proxypp/http/http_proxy_session.h"
#include "proxypp/proxy_server.hpp"
#include "proxypp/upstream_type.h"
#include "nul/uri.hpp"
#include <cassert>

namespace {
  struct HttpProxyServerContext {
    proxypp::ProxyServer server;
    proxypp::UpstreamType upstreamType{proxypp::UpstreamType::kUnknown};
    std::string upstreamServerHost;
    uint16_t upstreamServerPort;
    bool proxyRuleMode;
    std::shared_ptr<proxypp::ProxyRuleManager> proxyRuleManager{nullptr};
  };
}

namespace proxypp {
  HttpProxyServer::HttpProxyServer() : ctx_(new HttpProxyServerContext()) {
  }

  HttpProxyServer::~HttpProxyServer() {
    delete reinterpret_cast<HttpProxyServerContext *>(ctx_);
  }

  bool HttpProxyServer::start(const std::string &addr, uint16_t port, int backlog) {
    auto loop = std::make_shared<uvcpp::Loop>();
    if (!loop->init()) {
      LOG_E("Failed to start event loop");
      return false;
    }

    auto ctx = reinterpret_cast<HttpProxyServerContext *>(ctx_);
    ctx->server.setSessionCreator(
      [ctx](const std::shared_ptr<uvcpp::Tcp> &conn,
         const std::shared_ptr<nul::BufferPool> &bufferPool) {
        auto sess =
          std::make_shared<HttpProxySession>(std::move(conn), bufferPool);
        sess->setUpstreamServer(
          ctx->upstreamType, ctx->upstreamServerHost, ctx->upstreamServerPort);
        sess->setProxyRuleManager(ctx->proxyRuleManager);
        return sess;
      });

    if (!ctx->server.start(loop, addr, port, backlog)) {
      LOG_E("Failed to start start HttpProxyServerContext");
      return false;
    }
    loop->run();
    return true;
  }

  void HttpProxyServer::shutdown() {
    if (ctx_) {
      reinterpret_cast<HttpProxyServerContext *>(ctx_)->server.shutdown();
    }
  }

  bool HttpProxyServer::isRunning() {
    return ctx_ &&
      reinterpret_cast<HttpProxyServerContext *>(ctx_)->server.isRunning();
  }

  void HttpProxyServer::setEventCallback(EventCallback &&callback) {
    if (ctx_) {
      reinterpret_cast<HttpProxyServerContext *>(ctx_)->server.
        setEventCallback([callback](auto status, auto &message){
        callback(static_cast<ServerStatus>(status), message);
      });
    }
  }

  void HttpProxyServer::setUpstreamServer(const std::string &uriStr) {
    if (ctx_) {
      auto ctx = reinterpret_cast<HttpProxyServerContext *>(ctx_);
      nul::URI uri;
      if (!uri.parse(uriStr)) {
        LOG_W("Invalid upstream server ignored: %s", uriStr.c_str());
        return;
      }
      auto scheme = uri.getScheme();
      if (scheme == "socks5") {
        ctx->upstreamType = UpstreamType::kSOCKS5;

      } else if (scheme == "http" || scheme == "https") {
        ctx->upstreamType = UpstreamType::kHTTP;

      } else {
        LOG_W("Only 'socks5' or 'http' proxy server is support for upstream");
        return;
      }

      ctx->upstreamServerHost = uri.getHost();
      ctx->upstreamServerPort = uri.getPort();

      if (ctx->upstreamServerHost.empty()) {
        LOG_W("Invalid upstream server ignored: %s", uriStr.c_str());
        ctx->upstreamType = UpstreamType::kUnknown;
        return;
      }

      if (ctx->upstreamServerPort == 0) {
        LOG_W("Invalid upstream server port: %d", ctx->upstreamServerPort);
        ctx->upstreamType = UpstreamType::kUnknown;
        return;
      }
    }
  }

  void HttpProxyServer::setProxyRulesMode(bool blackListMode) {
    assert(ctx_);
    auto ctx = reinterpret_cast<HttpProxyServerContext *>(ctx_);
    if (ctx->proxyRuleManager) {
      ctx->proxyRuleManager->setProxyRulesMode(
        blackListMode ?
        proxypp::ProxyRuleManager::Mode::kBlackList :
        proxypp::ProxyRuleManager::Mode::kWhiteList);
    } else {
      ctx->proxyRuleManager = std::make_shared<proxypp::ProxyRuleManager>(
        blackListMode ?
        proxypp::ProxyRuleManager::Mode::kBlackList :
        proxypp::ProxyRuleManager::Mode::kWhiteList);
    }
  }

  void HttpProxyServer::addProxyRulesWithFile(
    const std::string &proxyRulesFile) {
    assert(ctx_);

    auto ctx = reinterpret_cast<HttpProxyServerContext *>(ctx_);
    if (!ctx->proxyRuleManager) {
      setProxyRulesMode(false);
    }
    ctx->proxyRuleManager->addProxyRulesWithFile(proxyRulesFile);
  }

  void HttpProxyServer::addProxyRulesWithString(
    const std::string &proxyRulesString) {
    assert(ctx_);

    auto ctx = reinterpret_cast<HttpProxyServerContext *>(ctx_);
    if (!ctx->proxyRuleManager) {
      setProxyRulesMode(false);
    }
    ctx->proxyRuleManager->addProxyRulesWithString(proxyRulesString);
  }

  void HttpProxyServer::addProxyRule(const std::string &regexStr) {
    assert(ctx_);

    auto ctx = reinterpret_cast<HttpProxyServerContext *>(ctx_);
    if (!ctx->proxyRuleManager) {
      setProxyRulesMode(false);
    }
    ctx->proxyRuleManager->addProxyRule(regexStr);
  }

  void HttpProxyServer::removeProxyRule(const std::string &regexStr) {
    assert(ctx_);
    reinterpret_cast<HttpProxyServerContext *>(ctx_)->proxyRuleManager->
      removeProxyRule(regexStr);
  }

  void HttpProxyServer::clearProxyRules() {
    assert(ctx_);
    reinterpret_cast<HttpProxyServerContext *>(ctx_)->proxyRuleManager->
      clearProxyRules();
  }

  void HttpProxyServer::addIgnoreRulesWithFile(
    const std::string &ignoreRulesFile) {
    assert(ctx_);
    reinterpret_cast<HttpProxyServerContext *>(ctx_)->proxyRuleManager->
      addIgnoreRulesWithFile(ignoreRulesFile);
  }

  void HttpProxyServer::addIgnoreRulesWithString(
    const std::string &ignoreRulesString) {
    assert(ctx_);
    reinterpret_cast<HttpProxyServerContext *>(ctx_)->proxyRuleManager->
      addIgnoreRulesWithString(ignoreRulesString);
  }

  void HttpProxyServer::addIgnoreRule(const std::string &regexStr) {
    assert(ctx_);
    reinterpret_cast<HttpProxyServerContext *>(ctx_)->proxyRuleManager->
      addIgnoreRule(regexStr);
  }

  void HttpProxyServer::removeIgnoreRule(const std::string &regexStr) {
    assert(ctx_);
    reinterpret_cast<HttpProxyServerContext *>(ctx_)->proxyRuleManager->
      removeIgnoreRule(regexStr);
  }

  void HttpProxyServer::clearIgnoreRules() {
    assert(ctx_);
    reinterpret_cast<HttpProxyServerContext *>(ctx_)->proxyRuleManager->
      clearIgnoreRules();
  }

} /* end of namspace: proxypp */

#ifdef BUILD_CLIENT 
#include "proxypp/cli/cmdline.h"

int main(int argc, char *argv[]) {
  cmdline::parser p;

  p.add<std::string>("host", 'h', "IPv4 or IPv6 address", false, "0.0.0.0");
  p.add<uint16_t>(
    "port", 'p', "port number", true, 0, cmdline::range(1, 65535));
  p.add<int>("backlog", 'b', "backlog for the server", false, 200, cmdline::range(1, 65535));
  p.add<std::string>(
    "upstream_server", 'u', "e.g. socks5://127.0.0.1:1080", false);
  p.add<std::string>(
    "proxy_rules_file", 'r', "regex proxy rules file, a rule for each line", false);
  p.add("proxy_rules_mode", 'm', "false=white_list_mode, true=black_list_mode");

  p.parse_check(argc, argv);

  proxypp::HttpProxyServer d{};
  auto upstreamServer = p.get<std::string>("upstream_server");
  if (!upstreamServer.empty()) {
    d.setUpstreamServer(upstreamServer);
  }

  d.setProxyRulesMode(p.exist("proxy_rules_mode"));
  d.addProxyRulesWithString("(?:.+\\.)?google\\.com\n(?:.+\\.)?youtube\\.com");
  d.removeProxyRule("(?:.+\\.)?google\\.com");

  //auto proxyRulesFile = p.get<std::string>("proxy_rules_file");
  //if (!proxyRulesFile.empty()) {
    //d.addProxyRulesWithFile(proxyRulesFile);
  //}

  d.start(
    p.get<std::string>("host"),
    p.get<uint16_t>("port"),
    p.get<int>("backlog"));
  return 0;
}
#endif
