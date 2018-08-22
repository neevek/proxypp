/*******************************************************************************
**          File: proxy_server.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 11:39 AM
**   Description: proxy server interface
*******************************************************************************/
#ifndef SOCKSPP_PROXY_SERVER_H_
#define SOCKSPP_PROXY_SERVER_H_
#include "sockspp/conn.h"
#include "uvcpp.h"
#include "nul/buffer_pool.hpp"

#include <string>
#include <functional>
#include <map>

namespace sockspp {
  template <typename ConnType>
  class ProxyServer {
    public:
      using ConnCreator = std::function<std::shared_ptr<ConnType>(
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
      using ConnId = uint32_t;

      ProxyServer(const std::shared_ptr<uvcpp::Loop> &loop) :
        server_(uvcpp::Tcp::createUnique(loop)) {
      }

      bool start(const std::string &addr, Port port, int backlog) {
        if (!createConn_) {
          LOG_E("ConnCreator is not set, call setConnCreator() first");
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

        for (auto &it : connections_) {
          it.second->close();
        }
        connections_.clear();
      }

      bool isRunning() const {
        return server_ && server_->isValid();
      }

      void setEventCallback(EventCallback &&callback) {
        eventCallback_ = callback;
      }

      void setConnCreator(ConnCreator connCreator) {
        createConn_ = connCreator;
      }


    private:
      void onClientConnected(std::unique_ptr<uvcpp::Tcp> conn) {
        auto connId = getNextConnId();
        conn->on<uvcpp::EvClose>([this, connId](const auto &e, auto &conn) {
          this->removeConn(connId);
        });

        //auto client = std::make_shared<ConnType>(std::move(conn), bufferPool_);
        auto client = createConn_(std::move(conn), bufferPool_);
        connections_[connId] = client;
        client->start();

        LOG_D("Connection count: %zu", connections_.size());
      }

      void removeConn(ConnId connId) {
        auto clientIt = connections_.find(connId);
        if (clientIt != connections_.end()) {
          auto client = std::move(clientIt->second);
          connections_.erase(clientIt);
        }
      }

      ConnId getNextConnId() {
        return ++connId;
      }

    private:
      std::unique_ptr<uvcpp::Tcp> server_{nullptr};
      std::shared_ptr<nul::BufferPool> bufferPool_{nullptr};
      std::map<ConnId, std::shared_ptr<ConnType>> connections_;
      EventCallback eventCallback_{nullptr};
      ConnId connId{0};

      ConnCreator createConn_{nullptr};
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_PROXY_SERVER_H_ */

