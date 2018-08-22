#include <gtest/gtest.h>
#include "sockspp/socks/socks_client.h"
#include "sockspp/socks/socks_proxy_session.h"
#include "sockspp/proxy_server.hpp"
#include "nul/util.hpp"

using namespace sockspp;

TEST(SocksTest, ProtocolTest) {
  auto loop = std::make_shared<uvcpp::Loop>();
  ASSERT_TRUE(loop->init());

  auto server = ProxyServer{};
  server.setSessionCreator([](std::unique_ptr<uvcpp::Tcp> &&tcpConn,
     const std::shared_ptr<nul::BufferPool> &bufferPool) {
    return std::make_shared<SocksProxySession>(std::move(tcpConn), bufferPool);
  });
  ASSERT_TRUE(server.start(loop, "0.0.0.0", 34567, 50));

  auto bufferPool = std::make_shared<nul::BufferPool>(100, 100);
  auto client = SocksClient{loop, bufferPool};
  ASSERT_TRUE(client.connect("0.0.0.0", 34567));

  client.once<EvSocksHandshake>([&](const auto &e, auto &client){
    ASSERT_TRUE(e.succeeded);
    client.template once<EvSocksRead>([&](const auto &e, auto &client){
      auto data = std::string{e.buf, static_cast<std::size_t>(e.nread)};
      LOG_D("data: %s", data.c_str());
      client.close();
      server.shutdown();
    });

    const auto httpGet = std::string{"GET / HTTP/1.1\r\n\r\n"};
    client.writeAsync(bufferPool->assembleDataBuffer(httpGet.c_str(), httpGet.length()));
  });
  client.startHandshake("www.github.com", 80);

  loop->run();
}

TEST(SocksTest, WithAuth) {
  auto loop = std::make_shared<uvcpp::Loop>();
  ASSERT_TRUE(loop->init());

  auto server = ProxyServer{};
  server.setSessionCreator([](std::unique_ptr<uvcpp::Tcp> &&tcpConn,
     const std::shared_ptr<nul::BufferPool> &bufferPool) {
    auto conn = std::make_shared<SocksProxySession>(std::move(tcpConn), bufferPool);
    conn->setUsername("user");
    conn->setPassword("password");
    return conn;
  });
  ASSERT_TRUE(server.start(loop, "0.0.0.0", 34567, 50));

  auto bufferPool = std::make_shared<nul::BufferPool>(100, 100);
  auto client = SocksClient{loop, bufferPool};
  client.setUsername("user");
  client.setPassword("password");
  ASSERT_TRUE(client.connect("0.0.0.0", 34567));

  client.once<EvSocksHandshake>([&](const auto &e, auto &client){
    ASSERT_TRUE(e.succeeded);
    client.template once<EvSocksRead>([&](const auto &e, auto &client){
      auto data = std::string{e.buf, static_cast<std::size_t>(e.nread)};
      LOG_D("data: %s", data.c_str());
      client.close();
      server.shutdown();
    });

    const auto httpGet = std::string{"GET / HTTP/1.1\r\n\r\n"};
    client.writeAsync(bufferPool->assembleDataBuffer(httpGet.c_str(), httpGet.length()));
  });
  client.startHandshake("www.github.com", 80);

  loop->run();
}

TEST(SocksTest, WithIncorrectCredential) {
  auto loop = std::make_shared<uvcpp::Loop>();
  ASSERT_TRUE(loop->init());

  auto server = ProxyServer{};
  server.setSessionCreator([](std::unique_ptr<uvcpp::Tcp> &&tcpConn,
     const std::shared_ptr<nul::BufferPool> &bufferPool) {
    auto conn = std::make_shared<SocksProxySession>(std::move(tcpConn), bufferPool);
    conn->setUsername("user");
    conn->setPassword("password");
    return conn;
  });
  ASSERT_TRUE(server.start(loop, "0.0.0.0", 34567, 50));

  auto bufferPool = std::make_shared<nul::BufferPool>(100, 100);
  auto client = SocksClient{loop, bufferPool};
  client.setUsername("user");
  client.setPassword("wrongpassword");
  ASSERT_TRUE(client.connect("0.0.0.0", 34567));

  client.once<EvSocksHandshake>([&](const auto &e, auto &client){
    ASSERT_FALSE(e.succeeded);
    client.close();
    server.shutdown();
  });
  client.startHandshake("www.github.com", 80);

  loop->run();
}
