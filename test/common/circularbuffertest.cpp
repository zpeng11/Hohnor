#include <gtest/gtest.h>
#include <functional>
#include <memory>
#include "hohnor/common/CircularBuffer.h"
using namespace Hohnor;

// Demonstrate some basic assertions.
TEST(circularBuffer, init) {
  // Expect two strings not to be equal.
  CircularBuffer<int> intBuffer(42);
  CircularBuffer<float> floatBuffer(42);
  CircularBuffer<std::shared_ptr<std::vector<int>>> objBuffer(42);
}

TEST(circularBuffer, sizecheckoverflow){
  CircularBuffer<int> intBuffer(5);
  for(int i =0 ; i < 5; i++) intBuffer.enqueue(42);
  EXPECT_EQ(5, intBuffer.size());
  try {
        intBuffer.enqueue(42);
        FAIL() << "Expected std::runtime_error";
    }
    catch(std::runtime_error const & err) {
        EXPECT_STREQ(err.what(),"buffer is full");
    }
    catch(...) {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(circularBuffer, checkOrder){
  CircularBuffer<int> intBuffer(100);
  for(int i =0 ; i < 100; i++) intBuffer.enqueue(i);
  for(int i =0 ; i < 100; i++) EXPECT_EQ(i, intBuffer.dequeue());
}