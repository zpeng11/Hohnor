#include "hohnor/time/Timestamp.h"
#include <gtest/gtest.h>

using namespace Hohnor;

TEST(TimestampTest, DefaultConstructor) {
    Timestamp ts;
    ASSERT_FALSE(ts.valid());
}

TEST(TimestampTest, ConstructorWithValue) {
    int64_t microseconds = 123456789;
    Timestamp ts(microseconds);
    ASSERT_TRUE(ts.valid());
    ASSERT_EQ(ts.microSecondsSinceEpoch(), microseconds);
}

TEST(TimestampTest, Now) {
    Timestamp now = Timestamp::now();
    ASSERT_TRUE(now.valid());
}

TEST(TimestampTest, ToString) {
    int64_t microseconds = 1678886400123456; // 2023-03-15 13:20:00.123456 UTC
    Timestamp ts(microseconds);
    ASSERT_EQ(ts.toString(), "1678886400.123456");
}

TEST(TimestampTest, ToFormattedString) {
    int64_t microseconds = 1678886400123456; // 2023-03-15 13:20:00.123456 UTC
    Timestamp ts(microseconds);
    // Note: This test is timezone-dependent. The result may vary.
    // We will assume a specific timezone for consistency, e.g., UTC.
    // To make it robust, we should test against GMT.
    ASSERT_EQ(ts.toFormattedString(true, Timestamp::GMT), "2023-03-15 13:20:00.123456");
    ASSERT_EQ(ts.toFormattedString(false, Timestamp::GMT), "2023-03-15 13:20:00");
}

TEST(TimestampTest, FromUnixTime) {
    time_t seconds = 1678886400;
    Timestamp ts = Timestamp::fromUnixTime(seconds);
    ASSERT_EQ(ts.secondsSinceEpoch(), seconds);
    ASSERT_EQ(ts.microSecondsSinceEpoch(), seconds * Timestamp::kMicroSecondsPerSecond);

    int microseconds = 123456;
    Timestamp ts2 = Timestamp::fromUnixTime(seconds, microseconds);
    ASSERT_EQ(ts2.secondsSinceEpoch(), seconds);
    ASSERT_EQ(ts2.microSecondsSinceEpoch(), seconds * Timestamp::kMicroSecondsPerSecond + microseconds);
}

TEST(TimestampTest, ComparisonOperators) {
    Timestamp ts1(1000);
    Timestamp ts2(2000);
    Timestamp ts3(1000);

    ASSERT_TRUE(ts1 < ts2);
    ASSERT_TRUE(ts1 <= ts2);
    ASSERT_TRUE(ts1 <= ts3);
    ASSERT_TRUE(ts2 > ts1);
    ASSERT_TRUE(ts2 >= ts1);
    ASSERT_TRUE(ts3 >= ts1);
    ASSERT_TRUE(ts1 == ts3);
    ASSERT_TRUE(ts1 != ts2);
}

TEST(TimestampTest, TimeDifference) {
    Timestamp ts1(1000000);
    Timestamp ts2(2500000);
    double diff = timeDifference(ts2, ts1);
    ASSERT_DOUBLE_EQ(diff, 1.5);
}

TEST(TimestampTest, AddTime) {
    Timestamp ts(1000000);
    double seconds_to_add = 2.5;
    Timestamp new_ts = addTime(ts, seconds_to_add);
    ASSERT_EQ(new_ts.microSecondsSinceEpoch(), 3500000);
}