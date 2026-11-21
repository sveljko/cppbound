#include <iostream>
#include "coblist.hpp"
#include "cobtesthelp.hpp"

#include <list>
#include <vector>
#include <iterator>
#include <ranges>
#include <algorithm>
#include <vector>
#include <type_traits>
#include <memory>
#include <string>
#include <list>
#include <functional>
#include <utility>
#include <optional>
#include <random>
#include <cassert>


template <class T, int N>
void print(coblist<T,N> const& l) {
    for (auto& x: l) {
        std::cout << x << std::endl;
    }
}

void basic()
{
    coblist<int, 5> l;

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
    coblist<int, 5> l;

    assert(l.push_front(1));
    assert(l.push_front(2));
    assert(l.push_front(3));
    assert(l.push_front(4));
    assert(l.push_front(5));

    auto r = l.erase(l.begin());
    assert(r == l.begin());
    assert(l.size() == 4);
    r = l.erase(++r);
    assert(3 == l.size());
    l.erase(++r);
    assert(2 == l.size());

    {
        int const exp[] = {4,2};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
}


void remove()
{
    coblist<int, 6> l;

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
    coblist<int, 9> l;

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
    coblist<int, 9> l;

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


void back()
{
    coblist<int, 4> l;

    CHECK(!l.pop_back<std::optional>().has_value());
    CHECK(l.lfront() == l.lend() && l.lback() == l.lend());
    CHECK(l.push_back(1));
    CHECK(l.lfront() == l.lback());
    CHECK(l.push_back(2));
    CHECK(l.push_front(0));
    CHECK(l.push_back(3));
    CHECK(!l.push_back(4));
    CHECK(l.max_size() == 4 && l.size() == 4);
    CHECK(same(l, std::list<int>{ 0, 1, 2, 3 }));
    CHECK(l.get(l.lfront()) == 0 && l.get(l.lback()) == 3);

    auto x = l.pop_back<std::optional>();
    CHECK(x.has_value() && *x == 3);
    x = l.pop_back<std::optional>();
    CHECK(x.has_value() && *x == 2);
    CHECK(same(l, std::list<int>{ 0, 1 }));
    CHECK(l.pop_back<std::optional>() == 1);
    CHECK(l.pop_back<std::optional>() == 0);
    CHECK(l.empty());
    CHECK(l.lfront() == l.lend() && l.lback() == l.lend());
}


// However the elements are taken out, there must be room for new ones
void recycle()
{
    for (int how = 0; how < 6; ++how) {
        coblist<int, 5> l;
        for (int round = 0; round < 20; ++round) {
            for (int i = 0; i < 5; ++i) {
                CHECK((i % 2) ? l.push_back(i) : l.push_front(i));
            }
            CHECK(!l.push_back(5));
            CHECK(!l.push_front(5));
            CHECK(l.insert(l.lbegin(), 5) == l.lend());
            CHECK(l.size() == 5);
            switch (how) {
            case 0:
                while (!l.empty()) {
                    l.pop_back<std::optional>();
                }
                break;
            case 1:
                while (!l.empty()) {
                    l.pop_front<std::optional>();
                }
                break;
            case 2:
                while (!l.empty()) {
                    l.erase(l.lback());
                }
                break;
            case 3:
                while (!l.empty()) {
                    l.erase(l.fwd(l.lfront()) != l.lend() ? l.fwd(l.lfront()) : l.lfront());
                }
                break;
            case 4:
                l.clear();
                break;
            case 5:
                CHECK(l.remove_if([](int) { return true; }) == 5);
                break;
            }
            CHECK(l.empty());
            CHECK(l.size() == 0);
        }
    }
}


void insert()
{
    coblist<int, 6> l;

    auto at = l.insert(l.lend(), 3);
    CHECK(at != l.lend() && l.get(at) == 3);
    const auto first = l.insert(l.lbegin(), 1);   // on its own: the order of evaluation of `==` is not specified
    CHECK(first == l.lfront());
    at = l.insert(at, 2);
    CHECK(l.get(at) == 2);
    CHECK(same(l, std::list<int>{ 1, 2, 3 }));
    CHECK(l.contains(at));
    CHECK(!l.contains(l.lend()));

    auto it = l.insert(l.cend(), 5);
    CHECK(*it == 5);
    it = l.insert(it, 4);
    CHECK(*it == 4);
    CHECK(l.contains(it));
    CHECK(same(l, std::list<int>{ 1, 2, 3, 4, 5 }));

    l.set(at, 22);
    CHECK(l.get(at) == 22);
    CHECK(same(l, std::list<int>{ 1, 22, 3, 4, 5 }));

    // links and iterators walk the same way
    auto lnk = l.lbegin();
    for (auto i = l.begin(); i != l.end(); ++i) {
        CHECK(l.get(lnk) == *i);
        lnk = l.fwd(lnk);
    }
    CHECK(lnk == l.lend());
    lnk = l.lback();
    for (int exp : { 5, 4, 3, 22, 1 }) {
        CHECK(l.get(lnk) == exp);
        lnk = l.bck(lnk);
    }
    CHECK(lnk == l.lend());

    auto nxt = l.erase(at);
    CHECK(l.get(nxt) == 3);
    CHECK(!l.contains(at));
    CHECK(l.erase(l.lback()) == l.lend());
    CHECK(l.erase(l.lend()) == l.lend());
    CHECK(same(l, std::list<int>{ 1, 3, 4 }));

    coblist<int, 6> const& cl = l;
    CHECK(cl.contains(cl.begin()));
    CHECK(!cl.contains(cl.end()));
    coblist<int, 6> other;
    CHECK(!other.contains(cl.begin()));
    CHECK(cl.begin() != other.begin());
    CHECK(*cl.cbegin() == 1);
}


void backwards()
{
    coblist<int, 5> l;
    CHECK(--l.end() == l.end());
    for (int i = 1; i <= 4; ++i) {
        CHECK(l.push_back(i));
    }
    auto e = l.end();
    --e;
    CHECK(*e == 4);
    --e;
    CHECK(*e == 3);
    auto b = l.begin();
    --b;
    CHECK(b == l.end());

    coblist<int, 5> const& cl = l;
    auto                   ce = cl.end();
    --ce;
    CHECK(*ce == 4);

    std::reverse(l.begin(), l.end());
    CHECK(same(l, std::list<int>{ 4, 3, 2, 1 }));
    CHECK(std::equal(std::make_reverse_iterator(l.end()), std::make_reverse_iterator(l.begin()),
                     std::begin({ 1, 2, 3, 4 })));
    CHECK(std::find(cl.begin(), cl.end(), 2) != cl.end());
    CHECK(std::distance(cl.begin(), cl.end()) == 4);
}


void predicates()
{
    coblist<int, 8> l;
    for (int i : { 5, 3, 8, 1, 9, 2, 7 }) {
        CHECK(l.push_back(i));
    }
    CHECK(l.remove_if([](int x) { return x > 7; }) == 2);
    CHECK(same(l, std::list<int>{ 5, 3, 1, 2, 7 }));
    l.sort([](int a, int b) { return a > b; });
    CHECK(same(l, std::list<int>{ 7, 5, 3, 2, 1 }));
    CHECK(l.unique([](int a, int b) { return (a % 2) == (b % 2); }) == 2);
    CHECK(same(l, std::list<int>{ 7, 2, 1 }));

    coblist<int, 8> empty;
    empty.sort();
    empty.reverse();
    CHECK(empty.unique() == 0);
    CHECK(empty.remove(1) == 0);
    CHECK(empty.empty());

    coblist<int, 8> copy = l;
    l.clear();
    CHECK(same(copy, std::list<int>{ 7, 2, 1 }));
    CHECK(copy.push_back(0));
    CHECK(l.empty());
}


// Random operations, compared to what `std::list` does
void model()
{
    std::mt19937 rng(20231114);
    for (int round = 0; round < 50; ++round) {
        coblist<int, 16> l;
        std::list<int>   ref;
        for (int op = 0; op < 500; ++op) {
            const int      v    = static_cast<int>(rng() % 10);
            const bool     room = ref.size() < 16;
            const unsigned at   = ref.empty() ? 0 : static_cast<unsigned>(rng() % (ref.size() + 1));
            switch (rng() % 12) {
            case 0:
                CHECK(l.push_front(v) == room);
                if (room) ref.push_front(v);
                break;
            case 1:
                CHECK(l.push_back(v) == room);
                if (room) ref.push_back(v);
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
                auto x = l.pop_back<std::optional>();
                CHECK(x.has_value() == !ref.empty());
                if (!ref.empty()) {
                    CHECK(*x == ref.back());
                    ref.pop_back();
                }
                break;
            }
            case 4:
            case 5: {
                auto lnk = l.lbegin();
                for (unsigned i = 0; i < at; ++i) {
                    lnk = l.fwd(lnk);
                }
                auto rslt = l.insert(lnk, v);
                CHECK((rslt != l.lend()) == room);
                if (room) {
                    CHECK(l.get(rslt) == v);
                    ref.insert(std::next(ref.begin(), at), v);
                }
                break;
            }
            case 6:
            case 7: {
                auto lnk = l.lbegin();
                for (unsigned i = 0; i < at; ++i) {
                    lnk = l.fwd(lnk);
                }
                auto rslt = l.erase(lnk);
                if (at < ref.size()) {
                    auto rit = ref.erase(std::next(ref.begin(), at));
                    CHECK((rslt == l.lend()) == (rit == ref.end()));
                    if (rit != ref.end()) CHECK(l.get(rslt) == *rit);
                }
                else {
                    CHECK(rslt == l.lend());
                }
                break;
            }
            case 8:
                CHECK(l.remove(v) == std::erase(ref, v));
                break;
            case 9: {
                const auto before = ref.size();
                ref.unique();
                CHECK(l.unique() == before - ref.size());
                break;
            }
            case 10:
                l.reverse();
                ref.reverse();
                break;
            case 11:
                l.sort();
                ref.sort();
                break;
            }
            CHECK(l.size() == ref.size());
            CHECK(l.empty() == ref.empty());
            CHECK(same(l, ref));
            CHECK(std::equal(ref.rbegin(), ref.rend(), std::make_reverse_iterator(l.end()),
                             std::make_reverse_iterator(l.begin())));
        }
    }
}


void stale()
{
    coblist<int, 5> l;
    CHECK(l.lend() == l.lbegin());
    CHECK(!l.live(l.lend()));
    cobint<0, 5> mid{};
    for (int i = 0; i < 5; ++i) {
        CHECK(l.push_back(i));
        if (i == 2) {
            mid = l.lback();
        }
    }
    CHECK(l.live(mid));
    auto n = l.erase(mid);
    CHECK(n != l.lend());
    CHECK(!l.live(mid));
    CHECK(l.erase(mid) == l.lend());
    CHECK(l.size() == 4);
    CHECK((std::vector<int>(l.begin(), l.end()) == std::vector<int>{ 0, 1, 3, 4 }));

    CHECK(l.insert(mid, 9) == l.lend());
    CHECK(!l.set(mid, 9));
    CHECK(l.set(l.lbegin(), 7));
    CHECK((std::vector<int>(l.begin(), l.end()) == std::vector<int>{ 7, 1, 3, 4 }));

    for (int r = 0; r < 20; ++r) {
        while (l.push_back(r)) {}
        CHECK(l.size() == 5);
        while (l.pop_front<std::optional>()) {}
        CHECK(l.empty());
    }
    CHECK(!l.live(l.lend()));
    for (int i = 0; i < 5; ++i) { l.push_back(i); }
    for (auto p = l.lbegin(); p != l.lend(); p = l.fwd(p)) {
        CHECK(l.live(p));
    }
}

static_assert(std::bidirectional_iterator<coblist<int, 5>::I>);
static_assert(std::bidirectional_iterator<coblist<int, 5>::CI>);
static_assert(std::ranges::bidirectional_range<coblist<int, 5>>);
static_assert(std::ranges::bidirectional_range<coblist<int, 5> const>);

void stditers()
{
    coblist<int, 5> l;
    for (int i = 1; i <= 4; ++i) { l.push_back(i); }
    std::ranges::reverse(l);
    CHECK((std::vector<int>(l.begin(), l.end()) == std::vector<int>{ 4, 3, 2, 1 }));
    CHECK(*std::ranges::find(l, 2) == 2);
    CHECK(std::ranges::find(l, 9) == l.end());
    auto b = l.begin();
    auto c = b++;
    CHECK(c == l.begin());
    CHECK(*b == 3);
    auto d = b--;
    CHECK(d != b);
    CHECK(l.begin() == l.cbegin());
    CHECK(l.cbegin() == l.begin());
    coblist<int, 5>::iterator n1, n2;
    CHECK(n1 == n2);
    CHECK(n1 != l.begin());
    CHECK(n1 != l.end());
    ++n1; --n1;
    *n1 = 5;
}


void objects()
{
    const int base = counted::alive;
    {
        coblist<counted, 6> l;
        for (int i = 0; i < 4; ++i) { CHECK(l.push_back(counted(i))); }
        CHECK(counted::alive == base + 4);
        CHECK(l.emplace_front(9));
        CHECK(counted::alive == base + 5);
        CHECK(l.pop_front<std::optional>()->v == 9);
        CHECK(counted::alive == base + 4);
        l.pop_back<std::optional>();
        CHECK(counted::alive == base + 3);
        l.erase(l.lfront());
        CHECK(counted::alive == base + 2);
        l.push_back(counted(5));
        l.push_back(counted(5));
        CHECK(l.unique([](counted const& a, counted const& b) { return a.v == b.v; }) == 1);
        CHECK(counted::alive == base + 3);
        CHECK(l.remove_if([](counted const& a) { return a.v == 5; }) == 1);
        CHECK(counted::alive == base + 2);

        coblist<counted, 6> c = l;
        CHECK(counted::alive == base + 4);
        CHECK(c.size() == l.size());
        for (auto p = l.lbegin(); p != l.lend(); p = l.fwd(p)) {
            CHECK(c.live(p) && (c.get(p).v == l.get(p).v));
        }
        coblist<counted, 6> d;
        d.push_back(counted(1));
        CHECK(counted::alive == base + 5);
        d = l;
        CHECK(counted::alive == base + 6);
        for (auto p = l.lbegin(); p != l.lend(); p = l.fwd(p)) {
            CHECK(d.live(p) && (d.get(p).v == l.get(p).v));
        }
        auto& dd = d;
        d = dd;
        CHECK(counted::alive == base + 6 && d.size() == 2);
        d.clear();
        CHECK(counted::alive == base + 4);
    }
    CHECK(counted::alive == base);

    {
        std::string a(40, 'a'), b(41, 'b'), c(42, 'c');
        coblist<std::string, 6> l;
        l.push_back(c); l.push_back(a); l.push_back(b); l.push_back(a);
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
        coblist<std::unique_ptr<int>, 3> l;
        CHECK(l.push_back(std::make_unique<int>(1)));
        CHECK(l.emplace_back(new int(2)));
        int seen = 0;
        CHECK(l.with(l.lfront(), [&](std::unique_ptr<int>& p) { seen = *p; }));
        CHECK(seen == 1);
        auto p = l.pop_front<std::optional>();
        CHECK(p.has_value() && **p == 1);
        l.for_each([&](std::unique_ptr<int>& q) { seen = *q; });
        CHECK(seen == 2);
    }

    {
        coblist<nodefault, 3> l;
        CHECK(l.emplace_back(1));
        CHECK(l.emplace_back(2));
        int sum = 0;
        l.with(l.lfront(), [&](nodefault& n) { sum += n.v; });
        l.for_each([&](nodefault const& n) { sum += n.v; });
        CHECK(sum == 4);
        CHECK(l.pop_front<std::optional>()->v == 1);
        l.erase(l.lfront());
        CHECK(l.empty());
    }

    {
        coblist<int, 4> l;
        l.push_back(1);
        l.push_back(2);
        l.push_back(3);
        auto it = ++l.begin();
        auto lk = l.lfront();
        lk = l.fwd(lk);
        l.erase(lk);
        int calls = 0;
        CHECK(!l.with(lk, [&](int&) { ++calls; }));
        CHECK(calls == 0);
        (void)*it; // does not crash
        CHECK(!l.live(lk));
    }
}


void usual()
{
    coblist<int, 5> l;
    CHECK(!l.full() && l.size() == 0);
    CHECK(!l.front<std::optional>().has_value());
    CHECK(!l.back<std::optional>().has_value());
    for (int i = 1; i <= 5; ++i) { l.push_back(i); }
    CHECK(l.full() && l.size() == 5);
    CHECK(l.front<std::optional>() == 1 && l.back<std::optional>() == 5);
    l.erase(l.lfront());
    CHECK(!l.full() && l.size() == 4);
    l.pop_back<std::optional>();
    CHECK(l.size() == 3);
    l.insert(l.lback(), 9);
    CHECK(l.size() == 4);
    l.remove(9);
    CHECK(l.size() == 3);

    // splice
    coblist<int, 6> a, b;
    for (int i : { 1, 2, 6 }) { a.push_back(i); }
    for (int i : { 3, 4, 5 }) { b.push_back(i); }
    CHECK(!a.splice(a.lbegin(), a));
    CHECK(a.splice(a.lback(), b));
    CHECK(b.empty() && a.size() == 6);
    CHECK((std::vector<int>(a.begin(), a.end()) == std::vector<int>{ 1, 2, 3, 4, 5, 6 }));
    coblist<int, 6> e;
    e.push_back(0);
    CHECK(!a.splice(a.lbegin(), e)); // no room
    CHECK(e.size() == 1);
    a.clear();
    a.push_back(8);
    b.push_back(7);
    CHECK(a.splice(a.lend(), b));
    CHECK(a.splice(a.lbegin(), e));
    CHECK((std::vector<int>(a.begin(), a.end()) == std::vector<int>{ 0, 8, 7 }));
    auto stale = a.lback();
    a.erase(stale);
    b.push_back(1);
    CHECK(!a.splice(stale, b)); // a stale link
    CHECK(b.size() == 1 && a.size() == 2);

    // merge
    coblist<int, 8> x, y;
    for (int i : { 1, 3, 5, 7 }) { x.push_back(i); }
    for (int i : { 2, 3, 4, 8 }) { y.push_back(i); }
    CHECK(x.merge(y));
    CHECK(y.empty());
    CHECK((std::vector<int>(x.begin(), x.end()) == std::vector<int>{ 1, 2, 3, 3, 4, 5, 7, 8 }));
    y.push_back(9);
    CHECK(!x.merge(y));
    CHECK(!x.merge(x));
    coblist<int, 4> p, q;
    p.push_back(5); p.push_back(3);
    q.push_back(4);
    CHECK(p.merge(q, std::greater<int>{}));
    CHECK((std::vector<int>(p.begin(), p.end()) == std::vector<int>{ 5, 4, 3 }));

    // swap
    coblist<int, 4> s, t;
    s.push_back(1); s.push_back(2);
    t.push_back(7);
    s.swap(t);
    CHECK((std::vector<int>(s.begin(), s.end()) == std::vector<int>{ 7 }));
    CHECK((std::vector<int>(t.begin(), t.end()) == std::vector<int>{ 1, 2 }));
}

void mergesort()
{
    // stable
    using P = std::pair<int, int>;
    coblist<P, 50> l;
    std::list<P> ref;
    std::mt19937 gen(7);
    for (int i = 0; i < 50; ++i) {
        P p{ static_cast<int>(gen() % 5), i };
        l.push_back(p);
        ref.push_back(p);
    }
    auto by_first = [](P const& a, P const& b) { return a.first < b.first; };
    l.sort(by_first);
    ref.sort(by_first);
    CHECK(std::equal(ref.begin(), ref.end(), l.begin(), l.end()));
    CHECK(l.size() == 50);
    // links are consistent both ways
    std::vector<int> back;
    for (auto it = l.end(); it != l.begin();) { --it; back.push_back(it->second); }
    std::reverse(back.begin(), back.end());
    std::vector<int> fwd;
    for (auto& x : l) { fwd.push_back(x.second); }
    CHECK(back == fwd);
    CHECK(l.lback() != l.lend());
    l.push_back(P{ 0, 99 }) ; // full list: must fail, tail still right
    CHECK(l.size() == 50);

    coblist<int, 1000> big;
    std::list<int> bref;
    for (int i = 0; i < 1000; ++i) {
        int v = static_cast<int>(gen() % 300);
        big.push_back(v);
        bref.push_back(v);
    }
    big.sort();
    bref.sort();
    CHECK(std::equal(bref.begin(), bref.end(), big.begin(), big.end()));
    for (int i = 0; i < 3; ++i) { big.pop_back<std::optional>(); bref.pop_back(); }
    CHECK(std::equal(bref.begin(), bref.end(), big.begin(), big.end()));
    coblist<int, 3> one, none;
    one.push_back(1);
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
    back();
    recycle();
    insert();
    backwards();
    predicates();
    model();

    return 0;
}
