#include <gtest/gtest.h>

#include "concurrentCollections/concurrentMap.h"

namespace kotlin_tracer {

using std::string;

TEST(ConcurrentMapTest, TestInsertGet) {
  ConcurrentCleanableMap<string, int *> map;
  int value = 1;
  map.insert("k", &value);
  EXPECT_EQ(1, *map.get("k"));
}

TEST(ConcurrentMapTest, TestContains) {
  ConcurrentCleanableMap<string, int *> map;
  int value = 1;
  map.insert("k", &value);
  EXPECT_EQ(true, map.contains("k"));
}

TEST(ConcurrentMapTest, TestContainsNegative) {
  ConcurrentCleanableMap<string, int *> map;
  EXPECT_EQ(false, map.contains("k"));
}

TEST(ConcurrentMapTest, TestGetNotFound) {
  ConcurrentCleanableMap<string, int *> map;
  EXPECT_EQ(nullptr, map.get("k"));
}

TEST(ConcurrentMapTest, TestGetCreateEntry) {
  ConcurrentCleanableMap<string, int *> map;
  EXPECT_EQ(nullptr, map.get("k"));
  EXPECT_EQ(true, map.contains("k"));
}

TEST(ConcurrentMapTest, TestAt) {
  ConcurrentCleanableMap<string, int *> map;
  int value = 1;
  map.insert("k", &value);
  EXPECT_EQ(1, *map.at("k"));
}

TEST(ConcurrentMapTest, TestAtNotFound) {
  ConcurrentCleanableMap<string, int *> map;
  bool error{false};
  try {
    map.at("k");
  } catch (...) {
    error = true;
  }
  EXPECT_TRUE(error);
}

TEST(ConcurrentMapTest, TestAtNotCreatingEntry) {
  ConcurrentCleanableMap<string, int *> map;
  try {
    map.at("k");
  } catch (...) {
  }
  EXPECT_FALSE(map.contains("k"));
}

TEST(ConcurrentMapTest, TestErase) {
  ConcurrentCleanableMap<string, int *> map;
  int value;
  map.insert("k", &value);
  EXPECT_TRUE(map.erase("k"));
  EXPECT_FALSE(map.contains("k"));
}

TEST(ConcurrentMapTest, TestEraseNegative) {
  ConcurrentCleanableMap<string, int *> map;
  int value;
  map.insert("k", &value);
  EXPECT_FALSE(map.erase("k2"));
  EXPECT_TRUE(map.contains("k"));
}

TEST(ConcurrentMapTest, TestCleanEmptyCase) {
  ConcurrentCleanableMap<string, int *> map;
  map.mark_current_values_for_clean();
  map.clean();
  EXPECT_TRUE(map.empty());
}

TEST(ConcurrentMapTest, TestClean) {
  ConcurrentCleanableMap<string, int *> map;
  int value1{0};
  int value2{1};
  map.insert("k1", &value1);
  map.insert("k2", &value2);
  EXPECT_FALSE(map.empty());
  map.mark_current_values_for_clean();
  map.clean();
  EXPECT_TRUE(map.empty());
  EXPECT_TRUE(map.cleanListEmpty());
}
}  //kotlin_tracer