#include "cobi.hpp"
#include "cobarray.hpp"

#include <iostream>


void ints()
{
    constexpr cobint<std::numeric_limits<int>::max() - 2> x;
    auto y = x + cobic<2>;
    // auto w = y + cobic<2>;

    cobint<std::numeric_limits<int>::lowest() + 2> xn;
    auto yn = xn - cobic<2>;
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
    arrays();

    return 0;
}
