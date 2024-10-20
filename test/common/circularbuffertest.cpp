#include <gtest/gtest.h>
// #include "hohnor/common/"

// Demonstrate some basic assertions.
TEST(circularBuffer, init) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}