#include <gtest/gtest.h>
#include <five/xrange.h>

TEST(Xrange, Basic) {
    i64 i = 0;
    for (auto j : five::xrange(10)) {
        EXPECT_EQ(i, j);
        i++;
    }
}
