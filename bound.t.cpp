#include "cobi.hpp"
#include "cobarray.hpp"
#include "cobmatrix.hpp"

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
    cobarray<int, 5> a;

    // a.get(cobic<6>);
    a.set(cobic<0>, 5);
    a.set(cobic<1>, 1);
    a.set(cobic<2>, 8);
    a.set(cobic<3>, 3);
    a.set(cobic<4>, 2);
    for (auto& x: a) {
        std::cout << x << " ";
    }
    std::cout << "\n";
    std::sort(a.begin(), a.end());
    for (auto& x: a) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    struct S {
        int i;
        float f;
    };
    cobarray<S, 3> as;
    as.set(cobic<0>, &S::f, 2.0f);
    assert(2.0 == as.get(cobic<0>, &S::f));
}


void arrange()
{
    cobarray<int, 4> a;

    a.set(cobic<0>, 2);
    a.set(cobic<1>, 4);
    a.set(cobic<2>, 3);
    a.set(cobic<3>, 9);

    for (auto x: a.irange()) {
	std::cout << a.get(x) << " ";
    }
    std::cout << std::endl;
}


void matrix()
{
    std::cout << "\nEnter the Matrix\n";
  
    cobmatrix<int, 3, 2> m;

    // m.get({cobic<4>, cobic<0>);
    // m.get({cobic<0>, cobic<3>);
    m.set({cobic<0>,cobic<0>}, 5);
    m.set({cobic<0>,cobic<1>}, 1);
    m.set({cobic<1>,cobic<0>}, 6);
    m.set({cobic<1>,cobic<1>}, 2);
    m.set({cobic<2>,cobic<0>}, 4);
    m.set({cobic<2>,cobic<1>}, 3);
    for (auto& x: m) {
        std::cout << x << " ";
    }
    std::cout << "\n";

    std::sort(m.begin(), m.end());
    for (auto& x: m) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
    
    struct S {
        int i;
        float f;
    };
    cobmatrix<S, 2, 2> sm;
    sm.set({cobic<0>, cobic<0>}, &S::f, 2.0f);
    assert(2.0 == sm.get({cobic<0>,cobic<0>}, &S::f));
}


int main()
{
    ints();
    intrange();
    arrays();
    arrange();
    matrix();

    return 0;
}
