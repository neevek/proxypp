#include <gtest/gtest.h>
#include "socks_client.h"
#include "socks_server.h"

using namespace sockspp;

TEST(SocksClient, ProtocolTest) {
  auto loop = std::make_shared<uvcpp::Loop>();
  ASSERT_TRUE(loop->init());

  auto server = SocksServer{"0.0.0.0", 34567, 50};
  ASSERT_TRUE(server.start(loop));

  auto bufferPool = std::make_shared<nul::BufferPool>(100, 100);
  auto client = SocksClient{loop, bufferPool};
  ASSERT_TRUE(client.connect("0.0.0.0", 34567));

  client.once<EvSocksHandshake>([](const auto &e, auto &client){
    ASSERT_TRUE(e.succeeded);
  });
  client.startHandshake("0.0.0.0", 9191);

  loop->run();
}
