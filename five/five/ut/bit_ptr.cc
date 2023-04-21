#include <gtest/gtest.h>
#include <five/type_name.h>
#include <tuple>
#include <iostream>
#include <five/bit_ptr.h>


struct S1 {
    int a = 1;
    int b = 2;
};

struct S2 {
    S2(int d_, int c_): d(d_), c(c_) {}
    double d = 3.;
    double c = 4.;
};


TEST(EitherPtr2, Basic) {
    using Ep = five::EitherPtr2<S1, S2>;
    auto p = Ep::construct<1>(5, 6);
    ASSERT_GE(1, p.bit_count());
    ASSERT_EQ(p.get_index(), 1);
    EXPECT_EQ(p.get<1>()->d, 5.);
    EXPECT_EQ(p.get<1>()->c, 6.);
}


TEST(EitherPtr2, ToFromRaw) {
    using Ep = five::EitherPtr2<S1, S2>;
    auto p = Ep::construct<1>(5, 6);

    void* raw = p.get_raw();

    auto p2 = Ep(raw);
    ASSERT_GE(1, p2.bit_count());
    ASSERT_EQ(p2.get_index(), 1);
    EXPECT_EQ(p2.get<1>()->d, 5.);
    EXPECT_EQ(p2.get<1>()->c, 6.);
}

struct Destroyable {
    static int destroyed;
    ~Destroyable() {
        destroyed++;
    }
};

int Destroyable::destroyed = 0;

TEST(EitherPtr2, Destroy) {

    Destroyable::destroyed = 0;
    using Ep = five::EitherPtr2<Destroyable, Destroyable>;
    {
        auto p = Ep::construct<1>();
    }
    ASSERT_EQ(1, Destroyable::destroyed);
}
