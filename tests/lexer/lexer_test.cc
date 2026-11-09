// Example test
// Author: Jiří Bína

#include <gtest/gtest.h>


TEST(ExampleTest, Assertions) {

  // Expect strings not equal
  EXPECT_STRNE("hello", "world");

  // Expect equal
  EXPECT_EQ(7 * 6, 42);
}
