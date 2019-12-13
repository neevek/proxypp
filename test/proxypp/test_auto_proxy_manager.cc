#include <gtest/gtest.h>
#include "proxypp/auto_proxy_manager.h"
#include "nul/util.hpp"

using namespace proxypp;

TEST(AutoProxyManager, Test1) {
  AutoProxyManager m;
  m.parseFileAsRules("/Users/neevek/Desktop/test.rule");

  EXPECT_TRUE(m.matches("google.com", 80));
  EXPECT_TRUE(m.matches("gmail.com", 80));
  EXPECT_TRUE(m.matches("wwww.gmail.com", 80));
  EXPECT_FALSE(m.matches("agmail.com", 80));
  EXPECT_FALSE(m.matches("fonts.googleapis.com", 80));
  EXPECT_TRUE(m.matches("googleapis.com", 80));
  EXPECT_TRUE(m.matches("www.googleapis.com", 80));
  EXPECT_FALSE(m.matches("www.translate.google.cn", 80));
  EXPECT_FALSE(m.matches("haha.google.cn", 80));
  EXPECT_FALSE(m.matches("translate.google.cn", 80));
  EXPECT_FALSE(m.matches("translate.google.cn", 443));
  EXPECT_TRUE(m.matches("groups.google.cn", 443));

}

