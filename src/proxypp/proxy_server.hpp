/*******************************************************************************
**          File: proxy_server.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 11:39 AM
**   Description: proxy server interface
*******************************************************************************/
#ifndef PROXYPP_PROXY_SERVER_H_
#define PROXYPP_PROXY_SERVER_H_
#include "proxypp/proxy_session.h"
#include "uvcpp.h"
#include "nul/buffer_pool.hpp"

#include <string>
#include <functional>
#include <map>

namespace proxypp {
  class ProxyServer final {
    public:
      using SessionCreator = std::function<std::shared_ptr<ProxySession>(
        std::unique_ptr<uvcpp::Tcp> &&conn,
        const std::shared_ptr<nul::BufferPool> &bufferPool)>;

      enum class ServerStatus {
        STARTED,
        SHUTDOWN,
        ERROR_OCCURRED
      };
      using EventCallback =
        std::function<void(ServerStatus event, const std::string& message)>;

      using Port = uint16_t;
      using SessionId = uint32_t;

      bool start(
        const std::shared_ptr<uvcpp::Loop> &loop,
        const std::string &addr, Port port, int backlog) {
        if (!createSession_) {
          LOG_E("SessionCreator is not set, call setSessionCreator() first");
          return false;
        }

        bufferPool_ = std::make_shared<nul::BufferPool>(8192, 20);
        server_ = uvcpp::Tcp::createUnique(loop, uvcpp::Tcp::Domain::INET);

        int on = 1;
        server_->setSockOption(
          SO_REUSEADDR, reinterpret_cast<void *>(&on), sizeof(on));
        // th following code causes crash on system with kernel version
        // lower than 3.9.0 when loop::run() is called
        //server_->setSockOption(
          //SO_REUSEPORT, reinterpret_cast<void *>(&on), sizeof(on));

        server_->on<uvcpp::EvError>([this, addr, port](const auto &e, auto &s) {
          LOG_E("ProxyServer failed to bind on %s:%d", addr.c_str(), port);
          if (this->eventCallback_) {
            this->eventCallback_(
              ServerStatus::ERROR_OCCURRED, std::string{uv_strerror(e.status)});
          }
        });
        server_->on<uvcpp::EvClose>([this, addr, port](const auto &e, auto &s) {
          LOG_I("ProxyServer [%s:%d] closed", addr.c_str(), port);
          if (this->eventCallback_) {
            this->eventCallback_(ServerStatus::SHUTDOWN, "ProxyServer shutdown");
          }
        });
        server_->on<uvcpp::EvAccept<uvcpp::Tcp>>([this](const auto &e, auto &s) {
          this->onClientConnected(
            std::move(const_cast<uvcpp::EvAccept<uvcpp::Tcp> &>(e).client));
        });

        if (server_->bind(addr, port) && server_->listen(backlog)) {
          LOG_I("ProxyServer bound on %s:%d", addr.c_str(), port);
          if (this->eventCallback_) {
            this->eventCallback_(
              ServerStatus::STARTED,
              "ProxyServer bound on " + addr + ":" + std::to_string(port));
          }
          return true;
        }
        return false;
      }

      void shutdown() {
        if (server_) {
          // must shutdown the Server from inside the loop
          auto work = uvcpp::Work::createShared(server_->getLoop());
          work->once<uvcpp::EvAfterWork>(
            [this, _ = work](const auto &e, auto &work) {
            server_->close();
          });
          work->start();
        }

        for (auto &it : sessions_) {
          it.second->close();
        }
        sessions_.clear();
      }

      bool isRunning() const {
        return server_ && server_->isValid();
      }

      void setEventCallback(EventCallback &&callback) {
        eventCallback_ = callback;
      }

      void setSessionCreator(SessionCreator sessionCreator) {
        createSession_ = sessionCreator;
      }

    private:
      void onClientConnected(std::unique_ptr<uvcpp::Tcp> conn) {
        auto sessionId = getNextSessionId();
        conn->on<uvcpp::EvClose>([this, sessionId](const auto &e, auto &conn) {
          this->removeSession(sessionId);
        });

        //auto client = std::make_shared<ProxySession>(std::move(conn), bufferPool_);
        auto client = createSession_(std::move(conn), bufferPool_);
        sessions_[sessionId] = client;
        client->start();

        LOG_D("Session count: %zu", sessions_.size());
      }

      void removeSession(SessionId sessionId) {
        auto clientIt = sessions_.find(sessionId);
        if (clientIt != sessions_.end()) {
          auto client = std::move(clientIt->second);
          sessions_.erase(clientIt);
        }
      }

      SessionId getNextSessionId() {
        return ++sessionId;
      }

    private:
      std::unique_ptr<uvcpp::Tcp> server_{nullptr};
      std::shared_ptr<nul::BufferPool> bufferPool_{nullptr};
      std::map<SessionId, std::shared_ptr<ProxySession>> sessions_;
      EventCallback eventCallback_{nullptr};
      SessionId sessionId{0};

      SessionCreator createSession_{nullptr};
  };
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_PROXY_SERVER_H_ */

