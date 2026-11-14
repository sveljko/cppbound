#include <iostream>
#include "cobfwdlist.hpp"
#include "cobtesthelp.hpp"

#include <forward_list>
#include <vector>
#include <iterator>
#include <ranges>
#include <algorithm>
#include <vector>
#include <type_traits>
#include <memory>
#include <string>
#include <forward_list>
#include <functional>
#include <utility>
#include <optional>
#include <random>
#include <cassert>


template <class T, int N>
void print(cobfwdlist<T,N> const& l) {
    for (auto& x: l) {
        std::cout << x << std::endl;
    }
}

void basic()
{
    cobfwdlist<int, 5> l;

    assert(l.empty());
    assert(l.size() == 0);
    assert(l.push_front(1));
    assert(!l.empty());
    assert(l.size() == 1);
    
    assert(l.push_front(2));
    assert(l.push_front(3));
    assert(l.push_front(4));
    assert(l.push_front(5));
    assert(l.size() == 5);
    assert(!l.push_front(6));

    {
        int const exp[] = {5,4,3,2,1};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
    l.reverse();
    {
        int const exp[] = {1,2,3,4,5};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
    l.reverse();

    auto x = l.pop_front<std::optional>();
    assert(x.has_value());
    assert(*x == 5);
    x = l.pop_front<std::optional>();
    assert(x.has_value() && (*x == 4));
    x = l.pop_front<std::optional>();
    assert(x.has_value() && (*x == 3));
    x = l.pop_front<std::optional>();
    assert(x.has_value() && (*x == 2));
    x = l.pop_front<std::optional>();
    assert(x.has_value() && (*x == 1));
    assert(!l.pop_front<std::optional>().has_value());
    assert(l.empty());

    assert(l.push_front(11));
    assert(l.push_front(12));
    assert(l.push_front(13));
    assert(l.push_front(14));
    {
        int const exp[] = {14,13,12,11};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
    l.reverse();
    {
        int const exp[] = {11,12,13,14};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
    l.clear();
    assert(l.empty());
}


void erase()
{
    cobfwdlist<int, 8> l;

    assert(l.push_front(1));
    assert(l.push_front(2));
    assert(l.push_front(3));
    assert(l.push_front(4));
    assert(l.push_front(5));
    assert(l.push_front(6));
    assert(l.push_front(7));
    assert(l.push_front(8));

    l.erase_after(l.begin());
    assert(l.size() == 7);
    auto r = l.erase_after(l.begin());
    assert(l.size() == 6);
    r = l.erase_after(r);
    assert(5 == l.size());
    ++r;
    l.erase_after(r);
    assert(4 == l.size());
    {
        int const exp[] = {8,5,3,2};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
}


void remove()
{
    cobfwdlist<int, 6> l;

    assert(l.push_front(11));
    assert(l.push_front(12));
    assert(l.push_front(11));
    assert(l.push_front(14));

    assert(2 == l.remove(11));
    assert(0 == l.remove(11));
    assert(1 == l.remove(12));

    assert(l.size() == 1);
    auto x = l.pop_front<std::optional>();
    assert(x.has_value());
    assert(*x == 14);
    assert(l.empty());
}


void unique()
{
    cobfwdlist<int, 9> l;

    assert(l.push_front(11));
    assert(l.push_front(11));
    assert(l.push_front(12));
    assert(l.push_front(14));
    assert(l.push_front(14));
    assert(l.push_front(15));
    assert(l.push_front(14));
    assert(l.push_front(14));
    assert(l.push_front(14));

    assert(4 == l.unique());
    assert(0 == l.unique());

    {
        int const exp[] = {14,15,14,12,11};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
}


void sort()
{
    cobfwdlist<int, 9> l;

    assert(l.push_front(11));
    assert(l.push_front(11));
    assert(l.push_front(12));
    assert(l.push_front(14));
    assert(l.push_front(14));
    assert(l.push_front(15));
    assert(l.push_front(14));
    assert(l.push_front(14));
    assert(l.push_front(14));

    l.sort();

    {
        int const exp[] = {11,11,12,14,14,14,14,14,15};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
}


template <class L, class R> bool same(L const& l, R const& r)
{
    return std::equal(l.begin(), l.end(), std::begin(r), std::end(r));
}


void insert()
{
    cobfwdlist<int, 5> l;

    // there is nothing to insert after
    CHECK(l.insert_after(l.lend(), 42) == l.lend());
    CHECK(l.insert_after(l.cend(), 42) == l.end());
    CHECK(l.empty());
    CHECK(l.size() == 0);

    CHECK(l.push_front(1));
    auto at = l.insert_after(l.lbegin(), 3);
    CHECK(at != l.lend());
    CHECK(l.insert_after(l.lbegin(), 2) != l.lend());
    auto it = l.insert_after(l.cbegin(), 0);
    CHECK(*it == 0);
    CHECK(l.contains(it));
    CHECK(same(l, std::forward_list<int>{ 1, 0, 2, 3 }));
    CHECK(l.insert_after(at, 4) != l.lend());
    CHECK(l.insert_after(at, 5) == l.lend());
    CHECK(l.size() == 5 && l.max_size() == 5);
    CHECK(same(l, std::forward_list<int>{ 1, 0, 2, 3, 4 }));

    CHECK(l.contains(at));
    CHECK(!l.contains(l.lend()));
    CHECK(l.erase_after(l.lend()) == l.lend());
    CHECK(l.erase_after(l.fwd(at)) == l.lend());
    CHECK(l.erase_after(at) == l.lend());
    CHECK(l.erase_after(l.lfront()) != l.lend());
    CHECK(same(l, std::forward_list<int>{ 1, 2, 3 }));

    cobfwdlist<int, 5> const& cl = l;
    CHECK(cl.contains(cl.begin()));
    CHECK(!cl.contains(cl.end()));
    CHECK(*cl.cbegin() == 1);
    CHECK(std::distance(cl.begin(), cl.end()) == 3);
    CHECK(std::find(cl.begin(), cl.end(), 3) != cl.end());

    cobfwdlist<int, 5> copy = l;
    l.clear();
    CHECK(same(copy, std::forward_list<int>{ 1, 2, 3 }));
}


// However the elements are taken out, there must be room for new ones
void recycle()
{
    for (int how = 0; how < 4; ++how) {
        cobfwdlist<int, 5> l;
        for (int round = 0; round < 20; ++round) {
            CHECK(l.push_front(0));
            for (int i = 1; i < 5; ++i) {
                CHECK((i % 2) ? l.push_front(i) : (l.insert_after(l.lbegin(), i) != l.lend()));
            }
            CHECK(!l.push_front(5));
            CHECK(l.insert_after(l.lbegin(), 5) == l.lend());
            CHECK(l.size() == 5);
            switch (how) {
            case 0:
                while (!l.empty()) {
                    l.pop_front<std::optional>();
                }
                break;
            case 1:
                while (l.fwd(l.lfront()) != l.lend()) {
                    l.erase_after(l.lfront());
                }
                l.pop_front<std::optional>();
                break;
            case 2:
                l.clear();
                break;
            case 3:
                CHECK(l.remove_if([](int) { return true; }) == 5);
                break;
            }
            CHECK(l.empty());
        }
    }
}


void predicates()
{
    cobfwdlist<int, 8> l;
    for (int i : { 7, 2, 9, 1, 8, 3, 5 }) {
        CHECK(l.push_front(i));
    }
    CHECK(l.remove_if([](int x) { return x > 7; }) == 2);
    CHECK(same(l, std::forward_list<int>{ 5, 3, 1, 2, 7 }));
    l.sort([](int a, int b) { return a > b; });
    CHECK(same(l, std::forward_list<int>{ 7, 5, 3, 2, 1 }));
    CHECK(l.unique([](int a, int b) { return (a % 2) == (b % 2); }) == 2);
    CHECK(same(l, std::forward_list<int>{ 7, 2, 1 }));

    cobfwdlist<int, 8> empty;
    empty.sort();
    empty.reverse();
    CHECK(empty.unique() == 0);
    CHECK(empty.remove(1) == 0);
    CHECK(empty.empty());
}


// Random operations, compared to what `std::forward_list` does
void model()
{
    std::mt19937 rng(20231114);
    for (int round = 0; round < 50; ++round) {
        cobfwdlist<int, 16>    l;
        std::forward_list<int> ref;
        unsigned               size = 0;
        for (int op = 0; op < 500; ++op) {
            const int      v    = static_cast<int>(rng() % 10);
            const bool     room = size < 16;
            const unsigned at   = size ? static_cast<unsigned>(rng() % size) : 0;
            switch (rng() % 9) {
            case 0:
            case 1:
                CHECK(l.push_front(v) == room);
                if (room) ref.push_front(v);
                break;
            case 2: {
                auto x = l.pop_front<std::optional>();
                CHECK(x.has_value() == !ref.empty());
                if (!ref.empty()) {
                    CHECK(*x == ref.front());
                    ref.pop_front();
                }
                break;
            }
            case 3: {
                auto lnk = l.lbegin();
                for (unsigned i = 0; i < at; ++i) {
                    lnk = l.fwd(lnk);
                }
                auto rslt = l.insert_after(lnk, v);
                CHECK((rslt != l.lend()) == (room && size));
                if (room && size) {
                    ref.insert_after(std::next(ref.begin(), at), v);
                }
                break;
            }
            case 4: {
                auto lnk = l.lbegin();
                for (unsigned i = 0; i < at; ++i) {
                    lnk = l.fwd(lnk);
                }
                l.erase_after(lnk);
                if (at + 1 < size) {
                    ref.erase_after(std::next(ref.begin(), at));
                }
                break;
            }
            case 5:
                CHECK(l.remove(v) == std::erase(ref, v));
                break;
            case 6:
                ref.unique();
                l.unique();
                break;
            case 7:
                l.reverse();
                ref.reverse();
                break;
            case 8:
                l.sort();
                ref.sort();
                break;
            }
            size = static_cast<unsigned>(std::distance(ref.begin(), ref.end()));
            CHECK(l.size() == size);
            CHECK(l.empty() == ref.empty());
            CHECK(same(l, ref));
        }
    }
}


void stale()
{
    cobfwdlist<int, 5> l;
    CHECK(!l.live(l.lend()));
    CHECK(l.push_front(4));
    for (int i = 3; i >= 0; --i) { CHECK(l.push_front(i)); }
    auto first = l.lbegin();
    auto mid = l.fwd(l.fwd(first)); // element 2
    auto before = l.fwd(first);
    CHECK(l.live(mid));
    CHECK(l.erase_after(before) != l.lend());
    CHECK(!l.live(mid));
    CHECK(l.size() == 4);
    CHECK(l.erase_after(mid) == l.lend());
    CHECK(l.insert_after(mid, 9) == l.lend());
    CHECK(l.size() == 4);
    CHECK((std::vector<int>(l.begin(), l.end()) == std::vector<int>{ 0, 1, 3, 4 }));

    for (int r = 0; r < 20; ++r) {
        while (l.push_front(r)) {}
        CHECK(l.size() == 5);
        while (l.pop_front<std::optional>()) {}
        CHECK(l.empty());
    }
    CHECK(!l.live(l.lend()));
    for (int i = 0; i < 5; ++i) { l.push_front(i); }
    for (auto p = l.lbegin(); p != l.lend(); p = l.fwd(p)) {
        CHECK(l.live(p));
    }
}

static_assert(std::forward_iterator<cobfwdlist<int, 5>::I>);
static_assert(std::forward_iterator<cobfwdlist<int, 5>::CI>);
static_assert(std::ranges::forward_range<cobfwdlist<int, 5>>);

void stditers()
{
    cobfwdlist<int, 5> l;
    for (int i = 4; i >= 1; --i) { l.push_front(i); }
    CHECK(*std::ranges::find(l, 2) == 2);
    CHECK(std::ranges::find(l, 9) == l.end());
    auto b = l.begin();
    auto c = b++;
    CHECK(c == l.begin());
    CHECK(*b == 2);
    CHECK(l.begin() == l.cbegin());
    CHECK(l.cbegin() == l.begin());
    cobfwdlist<int, 5>::iterator n1, n2;
    CHECK(n1 == n2);
    CHECK(n1 != l.begin());
    ++n1;
    *n1 = 5;
}


void objects()
{
    const int base = counted::alive;
    {
        cobfwdlist<counted, 6> l;
        for (int i = 0; i < 4; ++i) { CHECK(l.push_front(counted(i))); }
        CHECK(counted::alive == base + 4);
        CHECK(l.pop_front<std::optional>()->v == 3);
        CHECK(counted::alive == base + 3);
        l.erase_after(l.lfront());
        CHECK(counted::alive == base + 2);
        CHECK(l.emplace_after(l.lfront(), 7) != l.lend());
        CHECK(l.push_front(counted(5)));
        CHECK(l.push_front(counted(5)));
        CHECK(l.unique([](counted const& a, counted const& b) { return a.v == b.v; }) == 1);
        CHECK(l.remove_if([](counted const& a) { return a.v == 5; }) == 1);
        CHECK(counted::alive == base + 3);

        cobfwdlist<counted, 6> c = l;
        CHECK(counted::alive == base + 6);
        for (auto p = l.lbegin(); p != l.lend(); p = l.fwd(p)) {
            CHECK(c.live(p) && (c.get(p).v == l.get(p).v));
        }
        cobfwdlist<counted, 6> d;
        d.push_front(counted(1));
        d = l;
        CHECK(counted::alive == base + 9);
        auto& dd = d;
        d = dd;
        CHECK(counted::alive == base + 9 && d.size() == 3);
        d.clear();
        CHECK(counted::alive == base + 6);
    }
    CHECK(counted::alive == base);

    {
        std::string a(40, 'a'), b(41, 'b'), c(42, 'c');
        cobfwdlist<std::string, 6> l;
        l.push_front(c); l.push_front(a); l.push_front(b); l.push_front(a);
        l.sort();
        CHECK((std::vector<std::string>(l.begin(), l.end()) == std::vector<std::string>{ a, a, b, c }));
        CHECK(l.unique() == 1);
        l.reverse();
        CHECK((std::vector<std::string>(l.begin(), l.end()) == std::vector<std::string>{ c, b, a }));
        CHECK(l.remove(b) == 1);
        CHECK(l.pop_front<std::optional>() == c);
        CHECK(l.get(l.lfront()) == a);
    }

    {
        cobfwdlist<std::unique_ptr<int>, 3> l;
        CHECK(l.push_front(std::make_unique<int>(1)));
        CHECK(l.emplace_front(new int(2)));
        int seen = 0;
        CHECK(l.with(l.lfront(), [&](std::unique_ptr<int>& p) { seen = *p; }));
        CHECK(seen == 2);
        auto p = l.pop_front<std::optional>();
        CHECK(p.has_value() && **p == 2);
        l.for_each([&](std::unique_ptr<int>& q) { seen = *q; });
        CHECK(seen == 1);
    }

    {
        cobfwdlist<nodefault, 3> l;
        CHECK(l.emplace_front(1));
        CHECK(l.emplace_front(2));
        int sum = 0;
        l.with(l.lfront(), [&](nodefault& n) { sum += n.v; });
        l.for_each([&](nodefault const& n) { sum += n.v; });
        CHECK(sum == 5);
        CHECK(l.pop_front<std::optional>()->v == 2);
        CHECK(l.pop_front<std::optional>()->v == 1);
        CHECK(l.empty());
    }

    {
        cobfwdlist<int, 4> l;
        l.push_front(3);
        l.push_front(2);
        l.push_front(1);
        auto it = ++l.begin();
        auto lk = l.fwd(l.lfront());
        l.erase_after(l.lfront());
        int calls = 0;
        CHECK(!l.with(lk, [&](int&) { ++calls; }));
        CHECK(calls == 0);
        (void)*it; // does not crash
        CHECK(!l.live(lk));
    }
}


void usual()
{
    cobfwdlist<int, 5> l;
    CHECK(!l.full() && l.size() == 0);
    CHECK(!l.front<std::optional>().has_value());
    for (int i = 5; i >= 1; --i) { l.push_front(i); }
    CHECK(l.full() && l.size() == 5);
    CHECK(l.front<std::optional>() == 1);
    l.erase_after(l.lfront());
    CHECK(!l.full() && l.size() == 4);
    l.pop_front<std::optional>();
    CHECK(l.size() == 3);

    cobfwdlist<int, 8> x, y;
    for (int i : { 7, 5, 3, 1 }) { x.push_front(i); }
    for (int i : { 8, 4, 3, 2 }) { y.push_front(i); }
    CHECK(x.merge(y));
    CHECK(y.empty());
    CHECK((std::vector<int>(x.begin(), x.end()) == std::vector<int>{ 1, 2, 3, 3, 4, 5, 7, 8 }));
    y.push_front(9);
    CHECK(!x.merge(y));
    CHECK(!x.merge(x));
    cobfwdlist<int, 4> p, q, r;
    p.push_front(3); p.push_front(5);
    q.push_front(4);
    CHECK(p.merge(q, std::greater<int>{}));
    CHECK((std::vector<int>(p.begin(), p.end()) == std::vector<int>{ 5, 4, 3 }));
    r.push_front(1);
    CHECK(r.merge(q)); // merging nothing
    CHECK(r.size() == 1);
    cobfwdlist<int, 4> e;
    CHECK(e.merge(r));
    CHECK(e.size() == 1 && r.empty());

    cobfwdlist<int, 4> s, t;
    s.push_front(2); s.push_front(1);
    t.push_front(7);
    s.swap(t);
    CHECK((std::vector<int>(s.begin(), s.end()) == std::vector<int>{ 7 }));
    CHECK((std::vector<int>(t.begin(), t.end()) == std::vector<int>{ 1, 2 }));
}

void mergesort()
{
    using P = std::pair<int, int>;
    cobfwdlist<P, 50> l;
    std::forward_list<P> ref;
    std::mt19937 gen(7);
    for (int i = 0; i < 50; ++i) {
        P p{ static_cast<int>(gen() % 5), i };
        l.push_front(p);
        ref.push_front(p);
    }
    auto by_first = [](P const& a, P const& b) { return a.first < b.first; };
    l.sort(by_first);
    ref.sort(by_first);
    CHECK(std::equal(ref.begin(), ref.end(), l.begin(), l.end()));
    CHECK(l.size() == 50);

    cobfwdlist<int, 1000> big;
    std::forward_list<int> bref;
    for (int i = 0; i < 1000; ++i) {
        int v = static_cast<int>(gen() % 300);
        big.push_front(v);
        bref.push_front(v);
    }
    big.sort();
    bref.sort();
    CHECK(std::equal(bref.begin(), bref.end(), big.begin(), big.end()));
    cobfwdlist<int, 3> one, none;
    one.push_front(1);
    one.sort();
    none.sort();
    CHECK(one.size() == 1 && none.empty());
}


int main()
{
    usual(); mergesort();
    objects();
    stditers();
    stale();
    basic();
    erase();
    remove();
    unique();
    sort();
    insert();
    recycle();
    predicates();
    model();

    return 0;
}
