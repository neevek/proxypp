/*******************************************************************************
**          File: proxy_server.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 11:39 AM
**   Description: proxy server interface
*******************************************************************************/
#ifndef SOCKSPP_PROXY_SERVER_H_
#define SOCKSPP_PROXY_SERVER_H_
#include "sockspp/proxy_session.h"
#include "uvcpp.h"
#include "nul/buffer_pool.hpp"

#include <string>
#include <functional>
#include <map>

namespace sockspp {
  class ProxyServer {
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

      ProxyServer(const std::shared_ptr<uvcpp::Loop> &loop) :
        server_(uvcpp::Tcp::createUnique(loop)) {
      }

      bool start(const std::string &addr, Port port, int backlog) {
        if (!createSession_) {
          LOG_E("SessionCreator is not set, call setSessionCreator() first");
          return false;
        }

        bufferPool_ = std::make_shared<nul::BufferPool>(300, 60);
        if (server_) {
          server_->on<uvcpp::EvError>([this, addr, port](const auto &e, auto &server) {
            LOG_E("ProxyServer failed to bind on %s:%d", addr.c_str(), port);
            if (this->eventCallback_) {
              this->eventCallback_(
                ServerStatus::ERROR_OCCURRED, std::string{uv_strerror(e.status)});
            }
          });
          server_->on<uvcpp::EvClose>([this, addr, port](const auto &e, auto &server) {
            LOG_D("ProxyServer [%s:%d] closed", addr.c_str(), port);
            if (this->eventCallback_) {
              this->eventCallback_(ServerStatus::SHUTDOWN, "ProxyServer shutdown");
            }
          });
          server_->on<uvcpp::EvAccept<uvcpp::Tcp>>([this](const auto &e, auto &server) {
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
        }
        return false;
      }

      void shutdown() {
        if (server_) {
          // must shutdown the Server from inside the loop
          auto work = uvcpp::Work::createShared(server_->getLoop());
          work->ref(work);
          work->once<uvcpp::EvWork>([this](const auto &e, auto &work) {
            server_->close();
            work.unrefAll();
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
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_PROXY_SERVER_H_ */

