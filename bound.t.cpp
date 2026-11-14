#include "cobi.hpp"
#include "cobarray.hpp"
#include "cobmatrix.hpp"
#include "cobtesthelp.hpp"

#include <numeric>
#include <algorithm>
#include <iterator>
#include <ranges>

#include <iostream>
#include <string>
#include <sstream>
#include <type_traits>
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
    assert(4.0 == (as.grab(cobic<0>)->f = 4.0));
    assert(4.0 == as.grab(cobic<0>)->f);
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


void intcompare()
{
    cobint<0, 9> x;
    CHECK(x.be(3));
    CHECK(!x.be(10));
    CHECK(!x.be(-1));
    CHECK(x.get() == 3);
    CHECK(x.could(9) && !x.could(10));

    CHECK(x == 3);
    CHECK(x != 4);
    CHECK(x < 4 && !(x < 3));
    CHECK(x <= 3 && !(x <= 2));
    CHECK(x > 2 && !(x > 3));
    CHECK(x >= 3 && !(x >= 4));

    // the integer on the left hand side
    CHECK(3 == x);
    CHECK(4 != x);
    CHECK(2 < x && !(3 < x) && !(5 < x));
    CHECK(3 <= x && !(4 <= x));
    CHECK(4 > x && !(3 > x) && !(0 > x));
    CHECK(3 >= x && !(2 >= x));

    cobint<2, 20> y;
    CHECK(y.be(3));
    CHECK(x == y && !(x != y));
    CHECK(x <= y && x >= y && !(x < y) && !(x > y));
    CHECK(y.be(4));
    CHECK(x != y && x < y && x <= y && y > x && y >= x);

    // disjunct ranges are compared at compile time
    constexpr cobint<-5, -1> neg;
    constexpr cobint<0, 5>   pos;
    static_assert(neg != pos);
    static_assert(neg < pos);
    static_assert(!(pos < neg));
    static_assert(pos > neg);

    static_assert(cobint<0, 5>::greatest() == 5);
    static_assert(cobint<-3, 5>::smallest() == -3);
}


void intwalk()
{
    cobint<-2, 2> x;
    int           n = 0;
    do {
        ++n;
    } while (x.advance());
    CHECK(n == 5);
    CHECK(x == 2);
    do {
        --n;
    } while (x.ebb());
    CHECK(n == 0);
    CHECK(x == -2);

    cobint<-10, 10> wide;
    CHECK((wide = x) == -2);
    cobint<-10, 10> other;
    other = wide = cobic<7>;
    CHECK(other == 7);

    // a range with the greatest possible upper bound
    cobint<std::numeric_limits<int>::max() - 1, std::numeric_limits<int>::max()> top;
    CHECK(top.advance());
    CHECK(!top.advance());
    CHECK(top == std::numeric_limits<int>::max());
}


// All the values of two ranges, with the result checked to be right and in
// the range of the result.
template <int D1, int G1, int D2, int G2> void arithmetics()
{
    cobint<D1, G1> x;
    do {
        cobint<D2, G2> y;
        do {
            const auto s = x + y;
            CHECK(s.get() == x.get() + y.get());
            CHECK((s.get() >= s.DD) && (s.get() <= s.GG));
            const auto d = x - y;
            CHECK(d.get() == x.get() - y.get());
            CHECK((d.get() >= d.DD) && (d.get() <= d.GG));
            const auto p = x * y;
            CHECK(p.get() == x.get() * y.get());
            CHECK((p.get() >= p.DD) && (p.get() <= p.GG));
            const auto q = x / y;
            CHECK(q.get() == x.get() / y.get());
            CHECK((q.get() >= q.DD) && (q.get() <= q.GG));
            const auto m = x % y;
            CHECK(m.get() == x.get() % y.get());
            CHECK((m.get() >= m.DD) && (m.get() <= m.GG));
            const auto n = -x;
            CHECK(n.get() == -x.get());
            CHECK((n.get() >= n.DD) && (n.get() <= n.GG));
            const auto mx = max(x, y);
            CHECK(mx.get() == std::max(x.get(), y.get()));
            CHECK((mx.get() >= mx.DD) && (mx.get() <= mx.GG));
            const auto mn = min(x, y);
            CHECK(mn.get() == std::min(x.get(), y.get()));
            CHECK((mn.get() >= mn.DD) && (mn.get() <= mn.GG));
            const auto ab = abs(x);
            CHECK(ab.get() == std::abs(x.get()));
            CHECK((ab.get() >= ab.DD) && (ab.get() <= ab.GG));
        } while (y.advance());
    } while (x.advance());
}


void intarithmetics()
{
    arithmetics<0, 10, 3, 3>();
    arithmetics<0, 10, 1, 7>();
    arithmetics<-10, 10, 1, 7>();
    arithmetics<-10, 10, -7, -1>();
    arithmetics<-9, -2, -5, -3>();
    arithmetics<3, 20, -4, -2>();
    arithmetics<7, 7, 3, 3>();
    arithmetics<-7, -7, -3, -3>();

    // modulo of constants is exact
    static_assert((cobic<7> % cobic<3>).DD == 1);
    static_assert((cobic<7> % cobic<3>).GG == 1);
    static_assert((cobic<-7> % cobic<3>) == -1);
    static_assert((cobic<7> / cobic<-2>) == -3);
    static_assert((cobic<6> * cobic<-7>) == -42);

    // wide, but not overflowing
    constexpr auto big = cobint<-5, -1>{} - cobint<std::numeric_limits<int>::lowest()>{};
    static_assert(big.GG == std::numeric_limits<int>::max());

    // unsigned
    using ucobi = cobi<unsigned, 5u, 7u>;
    constexpr auto u = ucobi{} - cobi<unsigned, 1u, 2u>{};
    static_assert(u.DD == 3 && u.GG == 6 && u.get() == 4);
    constexpr auto usum = ucobi{} + cobi<unsigned, std::numeric_limits<unsigned>::max() - 7>{};
    static_assert(usum.GG == std::numeric_limits<unsigned>::max());
    constexpr auto uprod = ucobi{} * cobi<unsigned, 0u, 3u>{};
    static_assert(uprod.DD == 0 && uprod.GG == 21);
    constexpr auto uquot = cobi<unsigned, 0u, std::numeric_limits<unsigned>::max()>{}
                           / cobi<unsigned, 2u, std::numeric_limits<unsigned>::max()>{};
    static_assert(uquot.DD == 0 && uquot.GG == std::numeric_limits<unsigned>::max() / 2);

    // other integer types
    constexpr auto l = cobi<long long, 0, 1LL << 40>{} + cobi<long long, 1, 1LL << 40>{};
    static_assert(l.GG == 1LL << 41);
    constexpr auto c = cobi<signed char, -100, 100>{} - cobi<signed char, 0, 27>{};
    static_assert(c.DD == -127 && c.GG == 100);
}


void arrayiters()
{
    cobarray<int, 5> a;
    int              n = 0;
    for (auto i : a.irange()) {
        CHECK(a.set(i, n * 10) == n * 10);
        ++n;
    }
    CHECK(n == 5);
    CHECK(a.size() == 5 && a.max_size() == 5 && !a.empty());

    auto const b = a.begin();
    auto const e = a.end();
    CHECK(e - b == 5);
    CHECK(*(b + 2) == 20);
    CHECK(*(e - 1) == 40);
    CHECK((b + 5) == e);
    CHECK((e - 5) == b);
    CHECK((b + 2) - 2 == b);
    CHECK(b < e && b <= e && e > b && e >= b && !(e < b));

    // out of bounds lands at the end (the safety spot)
    CHECK((b + 6) == e);
    CHECK((b + 1000000) == e);
    CHECK((b - 1) == e);
    CHECK((b + (-1)) == e);
    CHECK((e - (-1)) == e);
    CHECK((e + (-5)) == b);
    CHECK((e - 6) == e);
    CHECK((b + std::numeric_limits<std::ptrdiff_t>::max()) == e);
    CHECK((b + std::numeric_limits<std::ptrdiff_t>::lowest()) == e);
    CHECK((e - std::numeric_limits<std::ptrdiff_t>::max()) == e);
    CHECK((e - std::numeric_limits<std::ptrdiff_t>::lowest()) == e);

    auto i = a.begin();
    --i;
    CHECK(i == b);
    i += 4;
    CHECK(*i == 40);
    ++i;
    CHECK(i == e);
    ++i;
    CHECK(i == e);
    i -= 5;
    CHECK(i == b);
    i = e;
    CHECK(i == e);
    *i = 77; // safety spot can be written to

    cobarray<int, 5> other;
    CHECK(a.begin() != other.begin());
    CHECK(!(a.begin() < other.end()));
    CHECK(a.begin() - other.begin() == 0);

    cobarray<int, 5> const& ca = a;
    CHECK(std::accumulate(ca.begin(), ca.end(), 0) == 100);
    CHECK(std::accumulate(ca.cbegin(), ca.cend(), 0) == 100);
    CHECK(*(ca.begin() + 3) == 30);
    CHECK((ca.end() - 6) == ca.end());
    CHECK(ca.end() - ca.begin() == 5);
    CHECK(std::is_sorted(ca.begin(), ca.end()));
    CHECK(std::find(ca.begin(), ca.end(), 30) - ca.begin() == 3);
    CHECK(std::find(ca.begin(), ca.end(), 31) == ca.end());
    CHECK(*std::lower_bound(ca.begin(), ca.end(), 25) == 30);

    std::reverse(a.begin(), a.end());
    CHECK(a.get(cobic<0>) == 40 && a.get(cobic<4>) == 0);
    std::sort(a.begin(), a.end());
    CHECK(a.get(cobic<0>) == 0 && a.get(cobic<4>) == 40);

    CHECK(a.maybe_set(4, 44));
    CHECK(!a.maybe_set(5, 55));
    CHECK(!a.maybe_set(-1, 55));
    CHECK(a.get(cobic<4>) == 44);

    cobarray<int, 5> c = a;
    CHECK(!(a != c));
    c.set(cobic<2>, -1);
    CHECK(a != c);
    c.fill(9);
    CHECK(std::count(c.begin(), c.end(), 9) == 5);

    struct S {
        int   i;
        float f;
    };
    cobarray<S, 2> as;
    as.set(cobic<1>, S{ 3, 0.5f });
    cobarray<S, 2> const& cas = as;
    CHECK(cas.grab(cobic<1>)->i == 3);
    CHECK(cas.get(cobic<1>, &S::f) == 0.5f);
    CHECK(as.set(cobic<1>, &S::i, 4) == 4);
    CHECK(cas.get(cobic<1>).i == 4);
}


void matrixiters()
{
    cobmatrix<int, 3, 4> m;
    CHECK(m.size() == 12 && m.max_size() == 12 && !m.empty());
    int n = 0;
    for (auto& x : m) {
        x = n++;
    }
    CHECK(n == 12);
    CHECK(m.get({ cobic<0>, cobic<0> }) == 0);
    CHECK(m.get({ cobic<1>, cobic<0> }) == 4);
    CHECK(m.get({ cobic<2>, cobic<3> }) == 11);

    auto const b = m.begin();
    auto const e = m.end();
    CHECK(e - b == 12);
    CHECK((b + 5) - b == 5);
    CHECK(b - (b + 5) == -5);
    CHECK(*(b + 5) == 5);
    CHECK(*(e - 1) == 11);
    CHECK(*(e + (-12)) == 0);
    CHECK((b + 12) == e);
    CHECK((e - 12) == b);
    CHECK(b < (b + 5) && (b + 5) > (b + 3) && (b + 4) >= (b + 4) && (b + 3) <= (b + 4));

    // out of bounds lands at the end (the safety spot)
    CHECK((b + 13) == e);
    CHECK((b - 1) == e);
    CHECK((b + (-1)) == e);
    CHECK((e - (-1)) == e);
    CHECK((e - 13) == e);
    CHECK((b + std::numeric_limits<std::ptrdiff_t>::lowest()) == e);
    CHECK((e - std::numeric_limits<std::ptrdiff_t>::lowest()) == e);

    auto i = m.begin();
    --i;
    CHECK(i == b);
    i += 11;
    CHECK(*i == 11);
    ++i;
    CHECK(i == e);
    ++i;
    CHECK(i == e);
    --i;
    CHECK(*i == 11);
    i -= 7;
    CHECK(*i == 4);
    --i;
    CHECK(*i == 3);

    cobmatrix<int, 3, 4> const& cm = m;
    CHECK(std::accumulate(cm.begin(), cm.end(), 0) == 66);
    CHECK(std::accumulate(cm.cbegin(), cm.cend(), 0) == 66);
    CHECK(*(cm.begin() + 7) == 7);
    CHECK(*(cm.end() - 2) == 10);
    CHECK((cm.begin() + 13) == cm.end());
    CHECK(cm.end() - cm.begin() == 12);
    CHECK(std::find(cm.begin(), cm.end(), 9) - cm.begin() == 9);

    std::reverse(m.begin(), m.end());
    CHECK(m.get({ cobic<0>, cobic<0> }) == 11);
    std::sort(m.begin(), m.end());
    CHECK(std::is_sorted(cm.begin(), cm.end()));

    CHECK(m.maybe_set(2, 3, 33));
    CHECK(!m.maybe_set(3, 0, 1));
    CHECK(!m.maybe_set(0, 4, 1));
    CHECK(!m.maybe_set(-1, 0, 1));
    CHECK(m.get({ cobic<2>, cobic<3> }) == 33);

    cobmatrix<int, 3, 4> c = m;
    CHECK(!(m != c));
    c.set({ cobic<1>, cobic<1> }, -1);
    CHECK(m != c);
    c.fill(9);
    CHECK(std::count(c.begin(), c.end(), 9) == 12);

    struct S {
        int   i;
        float f;
    };
    cobmatrix<S, 2, 2> sm;
    sm.set({ cobic<1>, cobic<0> }, S{ 3, 0.5f });
    sm.grab({ cobic<1>, cobic<0> })->i = 5;
    cobmatrix<S, 2, 2> const& csm = sm;
    CHECK(csm.grab({ cobic<1>, cobic<0> })->i == 5);
    CHECK(csm.get({ cobic<1>, cobic<0> }, &S::f) == 0.5f);
}


static_assert(std::random_access_iterator<cobarray<int, 5>::I>);
static_assert(std::random_access_iterator<cobarray<int, 5>::CI>);
static_assert(std::ranges::random_access_range<cobarray<int, 5>>);
static_assert(std::ranges::random_access_range<cobarray<int, 5> const>);
static_assert(std::random_access_iterator<cobmatrix<int, 3, 2>::I>);
static_assert(std::random_access_iterator<cobmatrix<int, 3, 2>::CI>);
static_assert(std::ranges::random_access_range<cobmatrix<int, 3, 2>>);
static_assert(std::ranges::random_access_range<cobmatrix<int, 3, 2> const>);
static_assert(std::forward_iterator<cobrng<int, 0, 5>>);
static_assert(std::forward_iterator<cobirange<int, 0, 5>>);

template <class C> void stdranges(C& a)
{
    std::ranges::sort(a);
    CHECK(std::ranges::is_sorted(a));
    std::ranges::reverse(a);
    CHECK(std::ranges::is_sorted(a, std::ranges::greater{}));
    CHECK(*std::ranges::find(a, 4) == 4);
    CHECK(std::ranges::find(a, 99) == a.end());
    CHECK(std::ranges::count(a, 4) == 1);

    auto b = a.begin();
    CHECK(b[1] == *(b + 1));
    CHECK(*(1 + b) == *(b + 1));
    auto c = b++;
    CHECK(c == a.begin());
    CHECK(b == a.begin() + 1);
    auto d = b--;
    CHECK(d == a.begin() + 1);
    CHECK(b == a.begin());
    CHECK(a.begin() == a.cbegin());
    CHECK(a.cbegin() == a.begin());
    CHECK(a.end() == a.cend());

    typename C::iterator n1;
    typename C::iterator n2;
    CHECK(n1 == n2);
    CHECK(n1 != a.begin());
    CHECK(n1 != a.end());
    ++n1;
    --n1;
    n1 += 5;
    *n1 = 3;
    *(n2 + 5) = 4;
    typename C::const_iterator n3;
    ++n3;
    CHECK(n3 != a.cbegin());
}

void stditers()
{
    cobarray<int, 5> a;
    int v = 3;
    for (auto& x : a) { x = v--; if (v < 0) v = 4; }
    std::ranges::sort(a);
    for (auto& x : a) { x = v++ % 5; }
    stdranges(a);
    cobmatrix<int, 3, 2> m;
    int k = 0;
    for (auto& x : m) { x = (k++ * 7) % 6; }
    stdranges(m);
}


static_assert(std::is_convertible_v<cobint<0, 3>, cobint<0, 5>>);
static_assert(!std::is_convertible_v<cobint<0, 5>, cobint<0, 3>>);
static_assert(std::is_assignable_v<cobint<0, 5>&, cobint<0, 3>>);
static_assert(!std::is_assignable_v<cobint<0, 3>&, cobint<0, 5>>);

void cobiextras()
{
    // three-way comparison
    cobint<0, 5>  a;
    cobint<3, 9>  b;
    CHECK(a.be(2));
    CHECK(b.be(4));
    CHECK((a <=> b) < 0);
    CHECK((b <=> a) > 0);
    CHECK((a <=> 2) == 0);
    CHECK((a <=> 1) > 0);
    CHECK((a <=> 3) < 0);
    CHECK(a < b);
    CHECK(a <= 2);
    CHECK(3 > a);
    CHECK(!(a >= b));

    // as
    cobi<long long, 1LL << 40> big;
    constexpr auto sum = cobint<0, 5>{}.as<long long>() + cobi<long long, 1LL << 40>{};
    static_assert(sum.GG == (1LL << 40) + 5);
    (void)big;
    constexpr auto uc = cobint<0, 255>{}.as<unsigned char>();
    static_assert(uc.GG == 255);

    // clamp and wrap
    using c9 = cobint<0, 9>;
    CHECK(c9::clamp(-7).get() == 0);
    CHECK(c9::clamp(99).get() == 9);
    CHECK(c9::clamp(3).get() == 3);
    cobint<-2, 2> w;
    CHECK(w.get() == -2);
    for (int k = -1; k <= 2; ++k) { w.advance_wrap(); CHECK(w.get() == k); }
    w.advance_wrap();
    CHECK(w.get() == -2);
    w.ebb_wrap();
    CHECK(w.get() == 2);
    for (int k = 1; k >= -2; --k) { w.ebb_wrap(); CHECK(w.get() == k); }

    // output
    cobint<-5, 5> n;
    CHECK(n.be(-3));
    std::ostringstream os;
    os << n;
    CHECK(os.str() == "-3");
    cobi<signed char, 0, 100> sc;
    CHECK(sc.be(65));
    std::ostringstream os2;
    os2 << sc;
    CHECK(os2.str() == "65");
}


void arrayobjects()
{
    constexpr cobarray<int, 3> c{ 1, 2, 3 };
    static_assert(c.get(cobic<1>) == 2);
    cobarray<int, 3> a{ 1, 2, 3 };
    CHECK(a.get(cobic<0>) == 1 && a.get(cobic<2>) == 3);

    cobarray<std::string, 2> s{ "x", std::string(40, 'y') };
    CHECK(s.get(cobic<0>) == "x");
    std::string long_one(50, 'z');
    s.set(cobic<1>, std::move(long_one));
    CHECK(s.get(cobic<1>) == std::string(50, 'z'));
    s.set(cobic<0>, std::string(45, 'q'));
    CHECK(s.grab(cobic<0>)->size() == 45);

    auto f = cobarray<int, 4>::filled(7);
    CHECK(std::count(f.begin(), f.end(), 7) == 4);
    auto g = cobarray<int, 4>::generate([](cobarray<int, 4>::index i) { return i.get() * 2; });
    CHECK(g.get(cobic<3>) == 6);
    constexpr auto cg = cobarray<int, 3>::filled(2);
    static_assert(cg.get(cobic<2>) == 2);

    auto mf = cobmatrix<int, 2, 3>::filled(4);
    CHECK(std::count(mf.begin(), mf.end(), 4) == 6);
    auto mg = cobmatrix<int, 2, 3>::generate([](cobmatrix<int, 2, 3>::index i) {
        return i.irow.get() * 10 + i.icolumn.get();
    });
    CHECK(mg.get({ cobic<1>, cobic<2> }) == 12);
    CHECK(mg.get({ cobic<0>, cobic<1> }) == 1);
    cobmatrix<std::string, 2, 2> ms;
    ms.set({ cobic<1>, cobic<1> }, std::string(40, 'm'));
    CHECK(ms.get({ cobic<1>, cobic<1> }).size() == 40);
}


void usualmembers()
{
    cobarray<int, 3> a{ 1, 2, 3 };
    cobarray<int, 3> b{ 1, 2, 3 };
    cobarray<int, 3> c{ 1, 2, 4 };
    CHECK(a == b);
    CHECK(a != c);
    CHECK(a.front() == 1 && a.back() == 3);
    a.swap(c);
    CHECK(a.back() == 4 && c.back() == 3);
    CHECK(c == b);
    constexpr cobarray<int, 2> ca{ 5, 6 };
    static_assert(ca.front() == 5 && ca.back() == 6);
    static_assert(ca == cobarray<int, 2>{ 5, 6 });

    using mat = cobmatrix<int, 2, 3>;
    static_assert(mat::rows() == 2 && mat::columns() == 3);
    mat m = mat::generate([](mat::index i) { return i.irow.get() * 10 + i.icolumn.get(); });
    mat n = m;
    CHECK(m == n);
    CHECK(m.front() == 0 && m.back() == 12);
    auto row = m.get_row(cobic<1>);
    CHECK(row.get(cobic<0>) == 10 && row.get(cobic<2>) == 12);
    n.set_row(cobic<0>, row);
    CHECK(m != n);
    CHECK(n.get({ cobic<0>, cobic<1> }) == 11);
    m.swap(n);
    CHECK(m.get({ cobic<0>, cobic<1> }) == 11);
    CHECK(n.get({ cobic<0>, cobic<1> }) == 1);
}


int main()
{
    usualmembers();
    arrayobjects();
    cobiextras();
    stditers();
    ints();
    intrange();
    arrays();
    arrange();
    matrix();
    intcompare();
    intwalk();
    intarithmetics();
    arrayiters();
    matrixiters();

    return 0;
}
