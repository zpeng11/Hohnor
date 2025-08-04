#include "hohnor/thread/Exception.h"
#include <gtest/gtest.h>
#include <string>

TEST(ExceptionTest, What)
{
    Hohnor::Exception ex("test");
    ASSERT_STREQ("test", ex.what());
}

TEST(ExceptionTest, StackTrace)
{
    Hohnor::Exception ex("test");
    ASSERT_GT(strlen(ex.stackTrace()), 0);
}