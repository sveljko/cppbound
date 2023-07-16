#include "cobi.hpp"
#include "cobarray.hpp"

#include <numeric>

#include <iostream>
#include <optional>
#include <cassert>


void ints()
{
    constexpr cobint<std::numeric_limits<int>::max() - 2> x;
    constexpr auto y = x + cobic<2>;
    static_assert(y.get() == std::numeric_limits<int>::max());
    // auto w = y + cobic<2>;

    constexpr cobint<std::numeric_limits<int>::lowest() + 2> xn;
    constexpr auto yn = xn - cobic<2>;
    static_assert(yn.get() == std::numeric_limits<int>::lowest());
    // auto w = yn - cobic<2>;

    constexpr cobint<0, 5> z;
    auto z2 = z + cobic<3>;
    static_assert(z2.DD == 3);
    static_assert(z2.GG == 8);

    // auto z0 = x / z;

    auto z3 = z - cobic<4>;
    static_assert(z3.DD == -4);
    static_assert(z3.GG == 1);

    // auto z0 = x / z3;

    // auto no = yn / cobic<-1>;

    static_assert(x != z);

    using id = cobint<0,4>;
    auto o1 = id::be<std::optional>(2);
    assert(o1 && o1->get() == 2);
    auto o2 = id::be<std::optional>(5);
    assert(!o2.has_value());
}


void intrange() 
{
    //constexpr cobint<std::numeric_limits<int>::max()> x;
    //constexpr auto rng = x.range();

    constexpr cobint<std::numeric_limits<int>::max()-1> y;
    constexpr auto rng = y.range();
    static_assert(++rng.begin() == rng.end());

    constexpr cobint<0,5> z;
    int s = 0;
    for (auto i: z.range()) {
        s += i;
    }
    assert(s == 15);
    auto a = std::accumulate(z.range().begin(), z.range().end(), 0);
    assert(s == a);

}


void arrays()
{
    cobarray<int, 3> a;

    // a.get(cobic<6>);
    a.set(cobic<0>, 5);
    a.set(cobic<1>, 1);
    a.set(cobic<2>, 8);
    for (auto& x: a) {
        std::cout << x << " ";
    }
    std::cout << "\n";
    std::sort(a.begin(), a.end());
    for (auto& x: a) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
}

int main()
{
    ints();
    intrange();
    arrays();

    return 0;
}
