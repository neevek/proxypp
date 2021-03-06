/*******************************************************************************
**          File: http_proxy_server.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 10:12 AM
**   Description: see the header file 
*******************************************************************************/
#include "proxypp/http/http_proxy_server.h"
#include "proxypp/http/http_proxy_session.h"
#include "proxypp/proxy_server.hpp"
#include "proxypp/auto_proxy_manager.h"
#include "proxypp/upstream_type.h"
#include "nul/uri.hpp"
#include <cassert>
#include <signal.h>

namespace {
  struct HttpProxyServerContext {
    proxypp::ProxyServer server;
    proxypp::UpstreamType upstreamType{proxypp::UpstreamType::kUnknown};
    std::string upstreamServerHost;
    uint16_t upstreamServerPort;
    bool proxyRuleMode;
    std::shared_ptr<proxypp::AutoProxyManager> autoProxyManager{nullptr};

    std::shared_ptr<uvcpp::Loop> loop;
    std::shared_ptr<uvcpp::FsEvent> proxyRuleFileChangeNotifier;

    std::chrono::system_clock::time_point lastUpdateProxyRuleTs;
  };
}

namespace proxypp {
  HttpProxyServer::HttpProxyServer() : ctx_(new HttpProxyServerContext()) {
    auto loop = std::make_shared<uvcpp::Loop>();
    if (!loop->init()) {
      LOG_E("Failed to start event loop");
      abort();
    }
    static_cast<HttpProxyServerContext *>(ctx_)->loop = loop;
  }

  HttpProxyServer::~HttpProxyServer() {
    delete static_cast<HttpProxyServerContext *>(ctx_);
  }

  bool HttpProxyServer::start(const std::string &addr, uint16_t port, int backlog) {
    auto ctx = static_cast<HttpProxyServerContext *>(ctx_);
    ctx->server.setSessionCreator(
      [ctx](const std::shared_ptr<uvcpp::Tcp> &conn,
         const std::shared_ptr<nul::BufferPool> &bufferPool) {
        auto sess =
          std::make_shared<HttpProxySession>(std::move(conn), bufferPool);
        sess->setUpstreamServer(
          ctx->upstreamType, ctx->upstreamServerHost, ctx->upstreamServerPort);
        sess->setAutoProxyManager(ctx->autoProxyManager);
        return sess;
      });

    if (!ctx->server.start(ctx->loop, addr, port, backlog)) {
      LOG_E("Failed to start start HttpProxyServerContext");
      return false;
    }
    ctx->loop->run();
    return true;
  }

  void HttpProxyServer::shutdown() {
    if (!ctx_) {
      return;
    }
    auto ctx = static_cast<HttpProxyServerContext *>(ctx_);
    ctx->server.shutdown();

    if (ctx->proxyRuleFileChangeNotifier) {
      auto work = uvcpp::Work::create(ctx->loop);
      work->once<uvcpp::EvAfterWork>(
        [ctx, _ = work](const auto &e, auto &work) {
          ctx->proxyRuleFileChangeNotifier->stop();
          ctx->proxyRuleFileChangeNotifier->close();
        });
      work->start();
    }
  }

  bool HttpProxyServer::isRunning() {
    return ctx_ &&
      static_cast<HttpProxyServerContext *>(ctx_)->server.isRunning();
  }

  void HttpProxyServer::setEventCallback(EventCallback &&callback) {
    if (ctx_) {
      static_cast<HttpProxyServerContext *>(ctx_)->server.
        setEventCallback([callback](auto status, auto &message){
        callback(static_cast<ServerStatus>(status), message);
      });
    }
  }

  void HttpProxyServer::setUpstreamServer(const std::string &uriStr) {
    if (!ctx_) {
      return;
    }

    auto ctx = static_cast<HttpProxyServerContext *>(ctx_);
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

    LOG_I("set upstream server: %s:%d",
          ctx->upstreamServerHost.c_str(), ctx->upstreamServerPort);
  }

  std::size_t HttpProxyServer::setAutoProxyRulesFile(
    const std::string &proxyRulesFile) {
    assert(ctx_);
    auto size = addAutoProxyRulesFile(proxyRulesFile);

    auto ctx = static_cast<HttpProxyServerContext *>(ctx_);
    if (size > 0 && !ctx->proxyRuleFileChangeNotifier) {
      LOG_I("will watch proxy rule file: %s", proxyRulesFile.c_str());

      ctx->proxyRuleFileChangeNotifier = uvcpp::FsEvent::create(ctx->loop);
      ctx->proxyRuleFileChangeNotifier->on<uvcpp::EvFsEvent>(
        [this, ctx, proxyRulesFile](const auto &e, auto &fsEvent){
          if (e.events == uvcpp::EvFsEvent::Event::kChange &&
              e.path == proxyRulesFile) {

            auto now = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
              now - ctx->lastUpdateProxyRuleTs).count();
            if (elapsed < 2) {
              return;
            }

            LOG_I("proxy rule file changed, will reload proxy rules from: %s",
                  proxyRulesFile.c_str());

            clearProxyRules();
            auto updatedSize = addAutoProxyRulesFile(proxyRulesFile);
            ctx->lastUpdateProxyRuleTs = std::chrono::system_clock::now();

            LOG_I("rules updated: %zu", updatedSize);
          }
      });
      ctx->proxyRuleFileChangeNotifier->start(
        proxyRulesFile, uvcpp::FsEvent::Flag::kWatchEntry);
    }

    return size;
  }

  std::size_t HttpProxyServer::addAutoProxyRulesFile(
    const std::string &proxyRulesFile) {
    if (proxyRulesFile.empty()) {
      return 0;
    }

    assert(ctx_);

    auto ctx = static_cast<HttpProxyServerContext *>(ctx_);
    if (!ctx->autoProxyManager) {
      ctx->autoProxyManager = std::make_shared<proxypp::AutoProxyManager>();
    }
    return ctx->autoProxyManager->parseFileAsRules(proxyRulesFile);
  }

  bool HttpProxyServer::addProxyRule(const std::string &rule) {
    assert(ctx_);

    auto ctx = static_cast<HttpProxyServerContext *>(ctx_);
    if (!ctx->autoProxyManager) {
      ctx->autoProxyManager = std::make_shared<proxypp::AutoProxyManager>();
    }
    return ctx->autoProxyManager->addRule(rule);
  }

  bool HttpProxyServer::removeProxyRule(const std::string &rule) {
    assert(ctx_);
    auto ctx = static_cast<HttpProxyServerContext *>(ctx_);
    return ctx->autoProxyManager && ctx->autoProxyManager->removeRule(rule);
  }

  void HttpProxyServer::clearProxyRules() {
    assert(ctx_);
    auto ctx = static_cast<HttpProxyServerContext *>(ctx_);
    if (ctx->autoProxyManager) {
      ctx->autoProxyManager->clearAll();
    }
  }

} /* end of namspace: proxypp */

//#ifdef BUILD_CLIENT 
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
    "proxy_rules_file", 'r', "auto proxy rule file", false);

  p.parse_check(argc, argv);

  proxypp::HttpProxyServer d{};
  auto upstreamServer = p.get<std::string>("upstream_server");
  if (!upstreamServer.empty()) {
    LOG_I("start server");
    d.setUpstreamServer(upstreamServer);
  }

  auto proxyRulesFile = p.get<std::string>("proxy_rules_file");
  if (!proxyRulesFile.empty()) {
    d.setAutoProxyRulesFile(proxyRulesFile);
  }

  signal(SIGPIPE, [](int){ /* ignore sigpipe */ });

  d.start(
    p.get<std::string>("host"),
    p.get<uint16_t>("port"),
    p.get<int>("backlog"));
  return 0;
}
//#endif
