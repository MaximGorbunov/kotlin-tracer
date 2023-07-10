#include <gtest/gtest.h>

#include "concurrentCollections/concurrentMap.h"

namespace kotlin_tracer {

using std::string;

TEST(ConcurrentMapTest, TestInsertGet) {
  ConcurrentMap<string, int *> map;
  int value = 1;
  map.insert("k", &value);
  EXPECT_EQ(1, *map.get("k"));
}

TEST(ConcurrentMapTest, TestContains) {
  ConcurrentMap<string, int *> map;
  int value = 1;
  map.insert("k", &value);
  EXPECT_EQ(true, map.contains("k"));
}

TEST(ConcurrentMapTest, TestContainsNegative) {
  ConcurrentMap<string, int *> map;
  EXPECT_EQ(false, map.contains("k"));
}

TEST(ConcurrentMapTest, TestGetNotFound) {
  ConcurrentMap<string, int *> map;
  EXPECT_EQ(nullptr, map.get("k"));
}

TEST(ConcurrentMapTest, TestGetCreateEntry) {
  ConcurrentMap<string, int *> map;
  EXPECT_EQ(nullptr, map.get("k"));
  EXPECT_EQ(true, map.contains("k"));
}

TEST(ConcurrentMapTest, TestAt) {
  ConcurrentMap<string, int *> map;
  int value = 1;
  map.insert("k", &value);
  EXPECT_EQ(1, *map.at("k"));
}

TEST(ConcurrentMapTest, TestAtNotFound) {
  ConcurrentMap<string, int *> map;
  bool error{false};
  try {
    map.at("k");
  } catch (...) {
    error = true;
  }
  EXPECT_TRUE(error);
}

TEST(ConcurrentMapTest, TestAtNotCreatingEntry) {
  ConcurrentMap<string, int *> map;
  try {
    map.at("k");
  } catch (...) {
  }
  EXPECT_FALSE(map.contains("k"));
}

TEST(ConcurrentMapTest, TestErase) {
  ConcurrentMap<string, int *> map;
  int value;
  map.insert("k", &value);
  EXPECT_TRUE(map.erase("k"));
  EXPECT_FALSE(map.contains("k"));
}


TEST(ConcurrentMapTest, TestEraseNegative) {
  ConcurrentMap<string, int *> map;
  int value;
  map.insert("k", &value);
  EXPECT_FALSE(map.erase("k2"));
  EXPECT_TRUE(map.contains("k"));
}
}  //kotlin_tracer