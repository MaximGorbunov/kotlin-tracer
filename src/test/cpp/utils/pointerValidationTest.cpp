#include <gtest/gtest.h>
#include "utils/pointerValidation.h"

namespace kotlin_tracer {
TEST(PointerValidationTest, TestNullPtr) {
  EXPECT_FALSE(is_valid(0));
}

TEST(PointerValidationTest, TestNonValidPtr) {
  EXPECT_FALSE(is_valid(-1));
}

TEST(PointerValidationTest, TestValidPtr) {
  int a = 1;
  EXPECT_TRUE(is_valid(reinterpret_cast<uint64_t>(&a)));
}
}  // namespace kotlin_tracer