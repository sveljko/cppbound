#include "atbarray.hpp"
#include "cobtesthelp.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

// A dimension can be set only once per tag per program
template <int K> struct tg {};

static_assert(!std::is_default_constructible_v<atbidx<tg<0>>>);
static_assert(!std::is_constructible_v<atbidx<tg<0>>, std::size_t>);
static_assert(std::random_access_iterator<atbarray<int, tg<0>>::I>);
static_assert(std::random_access_iterator<atbarray<int, tg<0>>::CI>);
static_assert(std::ranges::random_access_range<atbarray<int, tg<0>>>);
static_assert(std::ranges::random_access_range<atbarray<int, tg<0>> const>);

void dimension()
{
    CHECK(atbdim<tg<1>>::set(5));
    CHECK(!atbdim<tg<1>>::set(6));
    CHECK(atbdim<tg<1>>::get() == 5);

    // looked at before it was set
    CHECK(atbdim<tg<2>>::get() == 0);
    CHECK(!atbdim<tg<2>>::set(4));
    CHECK(atbdim<tg<2>>::get() == 0);
    CHECK(!atbidx<tg<2>>::be<std::optional>(0).has_value());
    CHECK(!atbidx<tg<2>>::first<std::optional>().has_value());
    CHECK(!atbidx<tg<2>>::last<std::optional>().has_value());
    atbarray<int, tg<2>> a;
    CHECK(a.empty() && a.size() == 0);
    CHECK(a.begin() == a.end());
    int loops = 0;
    for (auto i : atbarray<int, tg<2>>::irange()) {
        (void)i;
        ++loops;
    }
    CHECK(loops == 0);
}

void indices(int argc)
{
    using tag = tg<3>;
    const std::size_t n = static_cast<std::size_t>(argc) + 6;
    CHECK(atbdim<tag>::set(n));
    for (std::size_t i = 0; i < n; ++i) {
        auto x = atbidx<tag>::be<std::optional>(i);
        CHECK(x.has_value() && x->get() == i);
    }
    CHECK(!atbidx<tag>::be<std::optional>(n).has_value());
    CHECK(!atbidx<tag>::be<std::optional>(SIZE_MAX).has_value());

    auto first = *atbidx<tag>::first<std::optional>();
    auto last  = *atbidx<tag>::last<std::optional>();
    CHECK(first.get() == 0 && last.get() == n - 1);
    CHECK(first < last && first != last);
    CHECK(!last.advance() && last.get() == n - 1);
    CHECK(!first.ebb() && first.get() == 0);
    CHECK(first.advance() && first.get() == 1);
    CHECK(first.ebb() && first.get() == 0);
    std::size_t k = 0;
    while (first.advance()) { ++k; }
    CHECK(k == n - 1);

    std::size_t expect = 0;
    for (auto i : atbidx<tag>::range()) {
        CHECK(i.get() == expect);
        ++expect;
    }
    CHECK(expect == n);

    // two arrays of the same tag share indices
    atbarray<int, tag> a, b;
    for (auto i : atbarray<int, tag>::irange()) {
        a.set(i, static_cast<int>(i.get()));
        b.set(i, static_cast<int>(i.get()) * 2);
    }
    for (auto i : atbarray<int, tag>::irange()) {
        CHECK(b.get(i) == 2 * a.get(i));
    }
}

struct pt {
    int   i = 0;
    float f = 0;
};

void members()
{
    using tag = tg<4>;
    CHECK(atbdim<tag>::set(4));
    using arr = atbarray<int, tag>;
    auto at   = [](std::size_t i) { return *arr::index::be<std::optional>(i); };

    arr a;
    CHECK(a.size() == 4 && a.max_size() == 4 && !a.empty());
    CHECK(a.set(at(1), 7) == 7 && a.get(at(1)) == 7);
    CHECK(a.set(at(2), int{ 8 }) == 8);
    CHECK(a.maybe_set(0, 5) && a.get(at(0)) == 5);
    CHECK(!a.maybe_set(4, 5) && !a.maybe_set(SIZE_MAX, 5));
    arr const& ca = a;
    CHECK(*ca.grab(at(0)).operator->() == 5);

    a.fill(3);
    CHECK(std::count(a.begin(), a.end(), 3) == 4);
    auto f = arr::filled(9);
    CHECK(std::count(f.begin(), f.end(), 9) == 4);
    auto g = arr::generate([](arr::index i) { return static_cast<int>(i.get()) * 10; });
    CHECK(g.get(at(3)) == 30);
    CHECK(!(f == g) && (f == arr::filled(9)));

    // swap, deep copy, move
    f.swap(g);
    CHECK(f.get(at(3)) == 30 && g.get(at(3)) == 9);
    arr c = f;
    c.set(at(0), 99);
    CHECK(f.get(at(0)) == 0 && c.get(at(0)) == 99);
    c = f;
    CHECK(c == f);
    auto& self = c;
    c          = self;
    CHECK(c == f);
    auto m = std::move(f);
    CHECK(m.get(at(3)) == 30);
    CHECK(f.size() == 4 && f.get(at(3)) == 30); // still fully usable
    f.set(at(3), 1);
    CHECK(f.get(at(3)) == 1 && m.get(at(3)) == 30);

    // member pointers
    atbarray<pt, tag> p;
    p.set(at(1), &pt::i, 4);
    p.set(at(1), &pt::f, 0.5f);
    CHECK(p.get(at(1), &pt::i) == 4 && p.get(at(1), &pt::f) == 0.5f);
    p.grab(at(1))->i = 6;
    CHECK(p.get(at(1), &pt::i) == 6);

    // strings
    atbarray<std::string, tag> s;
    s.set(at(0), std::string(50, 'a'));
    std::string b(60, 'b');
    s.set(at(1), b);
    CHECK(s.get(at(0)).size() == 50 && s.get(at(1)) == b);
    auto s2 = s;
    s2.set(at(0), "x");
    CHECK(s.get(at(0)).size() == 50);
    s.fill(std::string(45, 'f'));
    CHECK(s.get(at(2)).size() == 45);
}

void iterators()
{
    using tag = tg<5>;
    CHECK(atbdim<tag>::set(5));
    using arr = atbarray<int, tag>;
    arr a;
    int v = 0;
    for (auto& x : a) { x = (v++ * 3) % 5; }

    auto b = a.begin();
    auto e = a.end();
    CHECK(e - b == 5);
    CHECK(b + 5 == e && b + 6 == e && b + 100 == e);
    CHECK(e - 5 == b && e - 6 == e);
    CHECK(*(b + 2) == b[2] && *(2 + b) == b[2]);
    auto c = b++;
    CHECK(c == a.begin() && b == a.begin() + 1);
    auto d = b--;
    CHECK(d == a.begin() + 1 && b == a.begin());
    --b;
    CHECK(b == a.begin());
    for (int i = 0; i < 10; ++i) { ++e; }
    CHECK(e == a.end());
    CHECK(a.begin() == a.cbegin() && a.cbegin() == a.begin());
    CHECK(a.begin() < a.end() && a.end() > a.begin());

    std::sort(a.begin(), a.end());
    CHECK(std::is_sorted(a.begin(), a.end()));
    std::ranges::sort(a, std::ranges::greater{});
    CHECK(std::ranges::is_sorted(a, std::ranges::greater{}));
    std::ranges::reverse(a);
    CHECK(*std::ranges::find(a, 4) == 4);
    CHECK(std::ranges::count(a, 4) == 1);

    arr::iterator       n1, n2;
    arr::const_iterator n3;
    CHECK(n1 == n2);
    CHECK(n1 != a.begin() && n1 != a.end());
    ++n1;
    --n1;
    n1 += 5;
    *n1 = 3;
    *(n2 + 5) = 4;
    ++n3;
    CHECK(n3 != a.cbegin());
}

void unsetiterators()
{
    using tag = tg<6>;
    atbarray<int, tag>::iterator       it;
    atbarray<int, tag>::const_iterator cit;
    *it = 1;
    ++it;
    --cit;
    CHECK(atbdim<tag>::set(3)); // the iterators did not freeze it
    CHECK(atbdim<tag>::get() == 3);
}

int main(int argc, char**)
{
    dimension();
    indices(argc);
    members();
    iterators();
    unsetiterators();

    return 0;
}
