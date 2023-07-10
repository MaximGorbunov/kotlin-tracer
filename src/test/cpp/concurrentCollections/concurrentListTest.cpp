#include <gtest/gtest.h>
#include "concurrentCollections/concurrentList.h"

namespace kotlin_tracer {

using std::string;

TEST(ConcurrentListTest, TestPopFront) {
  ConcurrentList<string *> list;
  auto first = string("1");
  list.push_front(&first);
  EXPECT_EQ(first, *list.pop_front());
}

TEST(ConcurrentListTest, TestPopFrontEmptyCase) {
  ConcurrentList<string *> list;
  EXPECT_EQ(nullptr, list.pop_front());
}

TEST(ConcurrentListTest, TestPushFront) {
  ConcurrentList<string *> list;
  auto second = string("2");
  auto first = string("1");
  list.push_front(&second);
  list.push_front(&first);
  EXPECT_EQ(first, *list.pop_front());
  EXPECT_EQ(second, *list.pop_front());
}

TEST(ConcurrentListTest, TestFind) {
  ConcurrentList<string *> list;
  auto second = string("2");
  auto first = string("1");
  list.push_front(&second);
  list.push_front(&first);
  std::function<bool(string *)> find_function = [](string *str) {
    return "2" == *str;
  };
  EXPECT_EQ(second, *list.find(find_function));
}
TEST(ConcurrentListTest, TestFindNotFound) {
  ConcurrentList<string *> list;
  auto second = string("2");
  auto first = string("1");
  list.push_front(&second);
  list.push_front(&first);
  std::function<bool(string *)> find_function = [](string *str) {
    return "3" == *str;
  };
  EXPECT_EQ(nullptr, list.find(find_function));
}

TEST(ConcurrentListTest, TestForEach) {
  ConcurrentList<string *> list;
  auto second = string("2");
  auto first = string("1");
  list.push_front(&second);
  list.push_front(&first);
  std::list<string *> new_list;
  std::function<void(string *)> for_each_function = [&new_list](string *str) {
    new_list.push_back(str);
  };
  list.forEach(for_each_function);
  EXPECT_EQ(2, new_list.size());
  EXPECT_EQ("1", *new_list.front());
  new_list.pop_front();
  EXPECT_EQ("2", *new_list.front());
}

TEST(ConcurrentListTest, TestBack) {
  ConcurrentList<string *> list;
  auto second = string("2");
  auto first = string("1");
  list.push_front(&second);
  list.push_front(&first);
  EXPECT_EQ(second, *list.back());
}

TEST(ConcurrentListTest, TestSize) {
  ConcurrentList<string *> list;
  auto second = string("2");
  auto first = string("1");
  list.push_front(&second);
  list.push_front(&first);
  EXPECT_EQ(2, list.size());
}

TEST(ConcurrentListTest, TestPushBack) {
  ConcurrentList<string *> list;
  auto second = string("2");
  auto first = string("1");
  list.push_back(&first);
  list.push_back(&second);
  EXPECT_EQ(first, *list.pop_front());
  EXPECT_EQ(second, *list.pop_front());
}

TEST(ConcurrentListTest, TestEmptyPositive) {
  ConcurrentList<string *> list;
  EXPECT_EQ(true, list.empty());
}

TEST(ConcurrentListTest, TestEmptyNegative) {
  ConcurrentList<string *> list;
  auto first = string("1");
  list.push_back(&first);
  EXPECT_EQ(false, list.empty());
}
}  //kotlin_tracer