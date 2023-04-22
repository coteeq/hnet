#include <gtest/gtest.h>
#include <five/time.h>

using namespace five;

TEST(Instant, Basic) {
    auto now = Instant::now();
    EXPECT_GT(now.nanos(), 0);
}

TEST(Instant, Diffs) {
    EXPECT_EQ(Instant(100) - Instant(50), Duration(50));
    EXPECT_EQ(Instant(100) + Duration(50), Instant(150));
    EXPECT_EQ(Instant(100) - Duration(50), Instant(50));
}

TEST(Instant, Repr) {
    auto fmt = [](i64 nanos) -> std::string {
        std::stringstream ss;
        ss << Duration(nanos);
        return ss.str();
    };
    EXPECT_EQ("42ns", fmt(42));
    EXPECT_EQ("2000ns", fmt(2000));
    EXPECT_EQ("2us", fmt(2001));
    EXPECT_EQ("2000us", fmt(2'000'000));
    EXPECT_EQ("2ms", fmt(2'000'001));
    EXPECT_EQ("2000ms", fmt(2'000'000'000));
    EXPECT_EQ("2s", fmt(2'000'000'001));
}
