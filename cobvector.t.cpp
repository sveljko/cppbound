#include "cobvector.hpp"
#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <limits>
#include <random>
#include <ranges>
#include <string>
#include <vector>

template <class T, int N> bool same(cobvector<T, N> const& v, std::vector<T> const& x)
{
    std::vector<T> got;
    v.for_each([&](T const& t) { got.push_back(t); });
    std::vector<T> byindex;
    for (auto i : v.irange()) {
        byindex.push_back(v.get(i));
    }
    return (got == x) && (byindex == x) && (v.size() == x.size());
}

void basics()
{
    cobvector<int, 5> v;
    CHECK(v.empty() && !v.full() && v.size() == 0 && v.max_size() == 5 && v.capacity() == 5);
    CHECK(!v.front<std::optional>().has_value() && !v.back<std::optional>().has_value());
    CHECK(!v.pop_back<std::optional>().has_value());
    int loops = 0;
    for (auto i : v.irange()) {
        (void)i;
        ++loops;
    }
    CHECK(loops == 0);

    CHECK(v.push_back(1) && v.push_back(2) && v.emplace_back(3));
    CHECK(same(v, { 1, 2, 3 }));
    CHECK(v.front<std::optional>() == 1 && v.back<perhaps>().value_or(0) == 3);

    // An index is within the capacity; whether there is an element is checked
    CHECK(v.live(cobic<2>) && !v.live(cobic<3>));
    CHECK(v.at<std::optional>(cobic<2>) == 3 && !v.at<std::optional>(cobic<3>).has_value());
    CHECK(v.get(cobic<1>) == 2 && v.get(cobic<4>) == 0);
    CHECK(v.set(cobic<1>, 20) && !v.set(cobic<3>, 40));
    CHECK(v.with(cobic<0>, [](int& x) { x = 10; }) && !v.with(cobic<4>, [](int&) { CHECK(false); }));
    cobvector<int, 5> const& cv = v;
    int                      seen = 0;
    CHECK(cv.with(cobic<1>, [&](int const& x) { seen = x; }) && seen == 20);
    CHECK(same(v, { 10, 20, 3 }));

    // an index that was checked at run time
    cobvector<int, 5>::index i;
    CHECK(i.be(2) && v.get(i) == 3 && !i.be(5));

    CHECK(v.swap(cobic<0>, cobic<2>) && !v.swap(cobic<0>, cobic<3>));
    CHECK(same(v, { 3, 20, 10 }));

    CHECK(v.pop_back<std::optional>() == 10);
    CHECK(same(v, { 3, 20 }));
    // an index kept from before is harmless
    CHECK(!v.live(cobic<2>) && v.get(cobic<2>) == 0 && !v.erase(cobic<2>));

    CHECK(v.push_back(4) && v.push_back(5) && v.push_back(6));
    CHECK(v.full() && !v.push_back(7) && !v.emplace_back(7) && !v.insert(cobic<0>, 7));
    CHECK(same(v, { 3, 20, 4, 5, 6 }));
    v.clear();
    CHECK(v.empty() && same(v, {}));
}

void insert_erase()
{
    cobvector<std::string, 6> v;
    CHECK(v.insert(cobic<0>, "c"));                 // into an empty one
    CHECK(!v.insert(cobic<2>, "x"));                // past the end
    CHECK(v.insert(cobic<1>, "e"));                 // at the end
    CHECK(v.insert(cobic<0>, "a"));                 // at the start
    CHECK(v.insert(cobic<1>, "b") && v.emplace(cobic<3>, 1u, 'd'));
    CHECK(same(v, { "a", "b", "c", "d", "e" }));
    CHECK(v.insert(cobic<5>, v.get(cobic<0>)) && v.full());
    CHECK(same(v, { "a", "b", "c", "d", "e", "a" }));
    CHECK(!v.insert(cobic<6>, "x") && !v.insert(cobic<3>, "x"));

    CHECK(v.erase(cobic<0>) && v.erase(cobic<4>) && v.erase(cobic<1>));
    CHECK(same(v, { "b", "d", "e" }));
    CHECK(!v.erase(cobic<3>));

    // what is put in may be an element of the vector
    v.with(cobic<2>, [&](std::string const& e) { CHECK(v.insert(cobic<0>, e)); });
    CHECK(same(v, { "e", "b", "d", "e" }));

    CHECK(v.resize(2) && same(v, { "e", "b" }));
    CHECK(v.resize(4, "z") && same(v, { "e", "b", "z", "z" }));
    CHECK(v.resize(5) && same(v, { "e", "b", "z", "z", "" }));
    CHECK(!v.resize(7) && v.size() == 5 && v.resize(6) && v.resize(0) && v.empty());
}

static_assert(std::random_access_iterator<cobvector<int, 5>::I>);
static_assert(std::random_access_iterator<cobvector<int, 5>::CI>);
static_assert(std::ranges::random_access_range<cobvector<int, 5>>);
static_assert(std::is_convertible_v<cobvector<int, 5>::I, cobvector<int, 5>::CI>);
static_assert(!std::is_convertible_v<cobvector<int, 5>::CI, cobvector<int, 5>::I>);

void iterators()
{
    cobvector<int, 8> v;
    CHECK(v.begin() == v.end() && v.cbegin() == v.cend());
    for (int x : { 5, 3, 8, 1 }) {
        v.push_back(x);
    }
    CHECK(v.end() - v.begin() == 4 && std::accumulate(v.begin(), v.end(), 0) == 17);
    int sum = 0;
    for (int& x : v) {
        sum += x;
        x *= 2;
    }
    CHECK(sum == 17 && same(v, { 10, 6, 16, 2 }));
    cobvector<int, 8> const& cv = v;
    CHECK(*cv.begin() == 10 && cv.begin()[2] == 16 && *(cv.end() - 1) == 2 && *(1 + cv.begin()) == 6);
    CHECK(v.begin() < v.end() && v.end() > v.begin() && v.begin() <= v.begin() && v.begin() + 4 >= v.end());

    std::sort(v.begin(), v.end());
    CHECK(same(v, { 2, 6, 10, 16 }));
    std::ranges::reverse(v);
    CHECK(same(v, { 16, 10, 6, 2 }) && (std::ranges::find(v, 6) - v.begin() == 2));
    CHECK(std::equal(std::make_reverse_iterator(v.end()), std::make_reverse_iterator(v.begin()), cv.begin() + 0)
          == false);

    // moving out of the vector gets one to the end, where the safety spot is
    auto it = v.begin();
    CHECK(it + 4 == v.end() && it + 5 == v.end() && it - 1 == v.end() && it + 1000 == v.end());
    CHECK(v.end() + 1 == v.end() && v.end() - 5 == v.end() && v.end() - 4 == v.begin());
    CHECK(it - std::numeric_limits<std::ptrdiff_t>::min() == v.end() && it + std::numeric_limits<std::ptrdiff_t>::min() == v.end());
    --it;
    CHECK(it == v.end());
    *v.end() = 77;                  // harmless: not an element
    CHECK(same(v, { 16, 10, 6, 2 }));
    *v.end() = 0;

    // an iterator kept from before the vector shrank is harmless too
    auto last = v.end() - 1;
    CHECK(*last == 2);
    v.pop_back<std::optional>();
    v.pop_back<std::optional>();
    CHECK(*last == 0 && last + 0 == v.end() && last - 1 == v.end());   // wherever it is moved, it is the end
    cobvector<int, 8>::I none;
    CHECK(*none == 0 && none != v.begin() && none - v.begin() == 0);

    // inserting and erasing
    auto at = v.insert(v.begin() + 1, 13);
    CHECK(at == v.begin() + 1 && *at == 13 && same(v, { 16, 13, 10 }));
    at = v.insert(v.end(), 4);
    CHECK(*at == 4 && same(v, { 16, 13, 10, 4 }));
    at = v.erase(v.begin());
    CHECK(at == v.begin() && *at == 13 && same(v, { 13, 10, 4 }));
    CHECK(v.erase(v.end()) == v.end());
    at = v.erase(v.begin() + 2);        // the last one: nothing follows it
    CHECK(at == v.end() && same(v, { 13, 10 }));
    cobvector<int, 8> other;
    other.push_back(1);
    CHECK(v.erase(other.begin()) == v.end() && v.insert(other.begin(), 9) == v.end() && same(v, { 13, 10 }));
    while (v.push_back(0)) {
    }
    CHECK(v.insert(v.begin(), 1) == v.end() && v.size() == 8);
    at = v.erase(std::remove(v.begin(), v.end(), 0), v.end());
    CHECK(at == v.end());
    CHECK(same(v, { 13, 10 }));
    for (int x : { 1, 2, 3, 4 }) {
        v.push_back(x);
    }
    CHECK(v.erase(v.begin() + 1, v.begin() + 4) == v.begin() + 1 && same(v, { 13, 3, 4 }));
    CHECK(v.erase(v.begin() + 1, v.begin() + 1) == v.begin() + 1 && same(v, { 13, 3, 4 }));     // an empty range
    CHECK(v.erase(v.begin() + 2, v.begin() + 1) == v.end() && v.erase(v.begin(), other.end()) == v.end());
    CHECK(same(v, { 13, 3, 4 }));
    at = v.erase(v.begin(), v.end());
    CHECK(at == v.end() && v.empty());

    cobvector<std::string, 4> names;
    names.push_back("b");
    names.push_back("a");
    std::ranges::sort(names);
    CHECK(names.begin()->size() == 1 && *names.begin() == "a");
}

void anytype()
{
    cobvector<nodefault, 3> nd;
    CHECK(nd.emplace_back(1) && nd.push_back(nodefault{ 2 }) && nd.insert(cobic<0>, nodefault{ 0 }));
    int sum = 0;
    nd.for_each([&](nodefault const& x) { sum = sum * 10 + x.v; });
    CHECK(sum == 12);

    cobvector<std::unique_ptr<int>, 4> up;
    CHECK(up.emplace_back(new int(1)) && up.push_back(std::make_unique<int>(3)));
    CHECK(up.insert(cobic<1>, std::make_unique<int>(2)));
    CHECK(up.erase(cobic<0>) && up.size() == 2);
    auto last = up.pop_back<std::optional>();
    CHECK(last.has_value() && **last == 3);
    CHECK(up.with(cobic<0>, [](std::unique_ptr<int>& p) { CHECK(*p == 2); }));

    counted::alive = 0;
    {
        cobvector<counted, 8> v;
        CHECK(counted::alive == 0);     // nothing is made before it is put in
        for (int i = 0; i < 8; ++i) {
            CHECK(v.emplace_back(i));
        }
        CHECK(counted::alive == 8 && !v.emplace_back(9) && counted::alive == 8);
        CHECK(v.erase(cobic<3>) && counted::alive == 7);
        CHECK(v.insert(cobic<0>, counted{ 50 }));
        CHECK(counted::alive == 8);
        v.pop_back<perhaps>();
        CHECK(counted::alive == 7);

        cobvector<counted, 8> c = v;
        CHECK(counted::alive == 14 && c.size() == 7);
        c.set(cobic<0>, counted{ 99 });
        CHECK(v.get(cobic<0>).v == 50 && c.get(cobic<0>).v == 99);
        c.clear();
        CHECK(counted::alive == 7);
        c = v;
        CHECK(counted::alive == 14);
        auto const& self = c;
        c                = self;
        CHECK(counted::alive == 14 && c.size() == 7);
        v.resize(2);
        CHECK(counted::alive == 9);
        v.swap(c);
        CHECK(counted::alive == 9 && v.size() == 7 && c.size() == 2);
    }
    CHECK(counted::alive == 0);

    cobvector<int, 3> a;
    cobvector<int, 3> b;
    CHECK(a == b);
    a.push_back(1);
    CHECK(!(a == b));
    b.push_back(1);
    CHECK(a == b);
    b.set(cobic<0>, 2);
    CHECK(a != b);
}

void against_std()
{
    std::mt19937 rng(20241117);
    for (int round = 0; round < 20; ++round) {
        cobvector<int, 16> v;
        std::vector<int>   model;
        for (int step = 0; step < 2000; ++step) {
            const int                   x = static_cast<int>(rng() % 1000);
            cobvector<int, 16>::position pos;
            cobvector<int, 16>::index    idx;
            const int                    at = static_cast<int>(rng() % 17);
            pos.be(at);
            const bool isidx = idx.be(at);
            switch (rng() % 6) {
            case 0: CHECK(v.push_back(x) == (model.size() < 16)); if (model.size() < 16) model.push_back(x); break;
            case 1: {
                const bool ok = (model.size() < 16) && (at <= static_cast<int>(model.size()));
                CHECK(v.insert(pos, x) == ok);
                if (ok) model.insert(model.begin() + at, x);
                break;
            }
            case 2: {
                const bool ok = isidx && (at < static_cast<int>(model.size()));
                CHECK(!isidx || (v.erase(idx) == ok));
                if (ok) model.erase(model.begin() + at);
                break;
            }
            case 3: {
                auto p = v.pop_back<std::optional>();
                CHECK(p.has_value() == !model.empty());
                if (p) { CHECK(*p == model.back()); model.pop_back(); }
                break;
            }
            case 4: {
                const bool ok = isidx && (at < static_cast<int>(model.size()));
                CHECK(!isidx || (v.set(idx, x) == ok));
                if (ok) model[at] = x;
                break;
            }
            default:
                if (isidx) {
                    const auto got = v.at<std::optional>(idx);
                    CHECK(got.has_value() == (at < static_cast<int>(model.size())));
                    if (got) CHECK(*got == model[at]);
                }
            }
            CHECK(same(v, model));
        }
    }
}

int main()
{
    basics();
    insert_erase();
    iterators();
    anytype();
    against_std();
    return 0;
}
