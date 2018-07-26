#include "socks_server.h"



#ifdef BUILD_CLIENT 
int main(int argc, const char *argv[]) {
  auto loop = std::make_shared<uvcpp::Loop>();
  if (!loop->init()) {
    return 1;
  }

  sockspp::SocksServer server;
  server.start(loop, "0.0.0.0", 9888, 200);

  loop->run();
  
  return 0;
}
#endif
