#include <gtest/gtest.h>
#include "socks_client.h"
#include "socks_server.h"

using namespace sockspp;

TEST(SocksTest, ProtocolTest) {
  auto loop = std::make_shared<uvcpp::Loop>();
  ASSERT_TRUE(loop->init());

  auto server = SocksServer{"0.0.0.0", 34567, 50};
  ASSERT_TRUE(server.start(loop));

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

  auto server = SocksServer{"0.0.0.0", 34567, 50};
  server.setUsername("user");
  server.setPassword("password");
  ASSERT_TRUE(server.start(loop));

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

  auto server = SocksServer{"0.0.0.0", 34567, 50};
  server.setUsername("user");
  server.setPassword("password");
  ASSERT_TRUE(server.start(loop));

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
