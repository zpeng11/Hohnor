#include "hohnor/time/Date.h"
#include <gtest/gtest.h>

using namespace Hohnor;

TEST(DateTest, DefaultConstructor) {
    Date d;
    ASSERT_FALSE(d.valid());
}

TEST(DateTest, ConstructorWithYmd) {
    Date d(2023, 3, 15);
    ASSERT_TRUE(d.valid());
    ASSERT_EQ(d.year(), 2023);
    ASSERT_EQ(d.month(), 3);
    ASSERT_EQ(d.day(), 15);
    ASSERT_EQ(d.weekDay(), 3); // Wednesday
}

TEST(DateTest, ToIsoString) {
    Date d(2023, 3, 15);
    ASSERT_EQ(d.toIsoString(), "2023-03-15");
}

TEST(DateTest, YearMonthDay) {
    Date d(2023, 3, 15);
    Date::YearMonthDay ymd = d.yearMonthDay();
    ASSERT_EQ(ymd.year, 2023);
    ASSERT_EQ(ymd.month, 3);
    ASSERT_EQ(ymd.day, 15);
}

TEST(DateTest, ComparisonOperators) {
    Date d1(2023, 3, 15);
    Date d2(2023, 3, 16);
    Date d3(2023, 3, 15);

    ASSERT_TRUE(d1 < d2);
    ASSERT_TRUE(d1 == d3);
    ASSERT_FALSE(d1 == d2);
}

TEST(DateTest, JulianDay) {
    Date d(1970, 1, 1);
    ASSERT_EQ(d.julianDayNumber(), Date::kJulianDayOf1970_01_01);
}