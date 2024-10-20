#include <gtest/gtest.h>
#include <functional>
#include <memory>
#include "hohnor/common/BinaryHeap.h"
using namespace Hohnor;

std::function<bool(int &, int &)> intLessThan = 
    [](int &lhs, int &rhs) ->bool{return lhs < rhs;};

std::function<bool(float &, float &)> floatLessThan = 
    [](float &lhs, float &rhs) ->bool{return lhs < rhs;};

struct mockStruct{
  mockStruct(int a_, float b_):a(a_),b(b_){}
  int a;
  float b;
};

std::function<bool(std::shared_ptr<mockStruct> &, std::shared_ptr<mockStruct> &)> structLessThan = 
    [](std::shared_ptr<mockStruct> &lhs, std::shared_ptr<mockStruct> &rhs) ->bool{
      if(lhs->a < rhs->a)
        return true;
      else if(lhs->a > rhs->a)
        return false;
      else
        return lhs->b < rhs->b;
      };

TEST(binaryHeap, init) {
  // Expect two strings not to be equal.
  BinaryHeap<int> intBH(intLessThan);
  BinaryHeap<float> floatBH(floatLessThan);
  BinaryHeap<std::shared_ptr<mockStruct>> structBH(structLessThan);
}

TEST(binaryHeap, putAndGet) {
  // Expect two strings not to be equal.
  BinaryHeap<int> intBH(intLessThan);
  const int test_num_int = 42;
  intBH.insert(test_num_int);
  EXPECT_EQ(test_num_int, intBH.popTop());

  BinaryHeap<float> floatBH(floatLessThan);
  const int test_num_float = 42.0;
  floatBH.insert(test_num_float);
  EXPECT_EQ(test_num_float, floatBH.popTop());

  BinaryHeap<std::shared_ptr<mockStruct>> structBH(structLessThan);
  auto test_struct_ptr = std::make_shared<mockStruct>(42, 42.0);
  structBH.insert(test_struct_ptr);
  EXPECT_EQ(test_struct_ptr.get(), structBH.popTop().get());
  
}

TEST(binaryHeap, checkorder) {
  // Expect two strings not to be equal.
  BinaryHeap<int> intBH(intLessThan);
  std::vector<int> list = {5,3,1,4,2};
  for(int n : list){
    intBH.insert(n);
  }
  int idx = 0;
  std::vector<int> compareList = {1,2,3,4,5};
  while(intBH.size()){
    EXPECT_EQ(compareList[idx], intBH.popTop());
    idx ++;
  }
}
