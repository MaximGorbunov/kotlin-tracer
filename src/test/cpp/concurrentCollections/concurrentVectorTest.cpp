#include <gtest/gtest.h>
#include "concurrentCollections/concurrentVector.h"

namespace kotlin_tracer {

using std::string;

TEST(ConcurrentVectorTest, InsertAndGet) {
  ConcurrentVector<string *> vector;
  auto first = string("1");
  vector.push_back(&first);
  EXPECT_EQ(first, *vector.at(0));
}

}  //kotlin_tracer