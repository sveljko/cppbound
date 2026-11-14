#include "cobtesthelp.hpp"

#include "cobskiplist.hpp"

#include <iterator>
#include <ranges>
#include <algorithm>
#include <vector>
#include <type_traits>
#include <memory>
#include <string>
#include <sstream>
#include <optional>
#include <random>
#include <set>
#include <cassert>


template <class T, int N>
void print(cobskiplist<T,N> const& l) {
    for (auto& x: l) {
        std::cout << x << ", ";
    }
    std::cout << std::endl;
}

void basic()
{
    cobskiplist<int, 8> l;

    assert(l.empty());
    assert(l.size() == 0);
    
    assert(l.insert(1).second);
    assert(!l.empty());
    assert(l.size() == 1);
    assert(!l.insert(1).second);

    std::cout.flush();

    assert(l.insert(2).second);
    assert(l.insert(3).second);
    assert(l.insert(4).second);
    assert(l.insert(5).second);
    assert(l.size() == 5);

    assert(l.insert(6).second);
    assert(l.insert(7).second);
    assert(l.insert(8).second);
    assert(!l.insert(9).second);
    assert(l.size() == 8);

    {
        int const exp[] = {1,2,3,4,5,6,7,8};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }

    assert(l.erase(555) == 0);
    assert(l.erase(3) == 1);
    assert(l.erase(1) == 1);
    assert(l.erase(8) == 1);
    assert(l.erase(4) == 1);
    assert(l.erase(2) == 1);
    assert(l.erase(5) == 1);
    assert(l.erase(6) == 1);
    assert(l.erase(7) == 1);

    assert(l.empty());
}



template <class L, class R> bool same(L const& l, R const& r)
{
    return std::equal(l.begin(), l.end(), std::begin(r), std::end(r));
}


void order()
{
    cobskiplist<int, 16> l;

    CHECK(l.find(1) == l.end());
    CHECK(l.count(1) == 0);
    CHECK(l.lower_bound(1) == l.end());
    CHECK(l.erase(1) == 0);
    CHECK(l.erase(l.lbegin()) == l.lend());

    for (int i : { 50, 30, 40, 10, 70, 20, 60 }) {
        auto r = l.insert(i);
        CHECK(r.second);
        CHECK(*r.first == i);
    }
    CHECK(same(l, std::set<int>{ 10, 20, 30, 40, 50, 60, 70 }));
    CHECK(l.size() == 7 && l.max_size() == 16);

    auto dup = l.insert(40);
    CHECK(!dup.second);
    CHECK(*dup.first == 40);
    CHECK(l.size() == 7);

    CHECK(l.count(10) == 1 && l.count(70) == 1 && l.count(40) == 1);
    CHECK(l.count(5) == 0 && l.count(45) == 0 && l.count(75) == 0);
    CHECK(*l.find(10) == 10 && *l.find(70) == 70);
    CHECK(l.find(5) == l.end() && l.find(45) == l.end() && l.find(75) == l.end());
    CHECK(*l.lower_bound(5) == 10);
    CHECK(*l.lower_bound(10) == 10);
    CHECK(*l.lower_bound(11) == 20);
    CHECK(*l.lower_bound(70) == 70);
    CHECK(l.lower_bound(71) == l.end());

    cobskiplist<int, 16> const& cl = l;
    CHECK(*cl.find(30) == 30);
    CHECK(cl.find(31) == cl.end());
    CHECK(*cl.lower_bound(31) == 40);
    CHECK(cl.contains(cl.find(30)));
    CHECK(!cl.contains(cl.end()));
    CHECK(*cl.cbegin() == 10);

    // by link
    auto lnk = l.lbegin();
    CHECK(l.contains(lnk));
    CHECK(!l.contains(l.lend()));
    lnk = l.fwd(lnk);
    auto after = l.fwd(lnk);
    CHECK(l.erase(lnk) == after);
    CHECK(!l.contains(lnk));
    CHECK(l.erase(lnk) == l.lend()); // not in any more
    CHECK(same(l, std::set<int>{ 10, 30, 40, 50, 60, 70 }));

    // by iterator
    auto it = l.erase(l.find(70));
    CHECK(it == l.end());
    it = l.erase(l.find(10));
    CHECK(*it == 30);
    CHECK(l.erase(l.end()) == l.end());
    CHECK(same(l, std::set<int>{ 30, 40, 50, 60 }));
    cobskiplist<int, 16> other;
    CHECK(other.insert(30).second);
    CHECK(l.erase(other.begin()) == l.end()); // not ours
    CHECK(l.size() == 4);

    l.clear();
    CHECK(l.empty());
    CHECK(l.size() == 0);
    CHECK(l.lbegin() == l.lend());
    CHECK(l.begin() == l.end());
}


// Erased elements must make room for new ones
void recycle()
{
    cobskiplist<int, 8> l;
    for (int round = 0; round < 50; ++round) {
        for (int i = 0; i < 8; ++i) {
            CHECK(l.insert((i * 5 + round) % 8).second);
        }
        CHECK(!l.insert(100).second);
        CHECK(l.insert(100).first == l.end());
        CHECK(!l.insert(3).second);
        CHECK(*l.insert(3).first == 3);
        CHECK(l.size() == 8);
        if (round % 2) {
            l.clear();
        }
        else {
            for (int i = 0; i < 8; ++i) {
                CHECK(l.erase(i) == 1);
            }
        }
        CHECK(l.empty());
    }
}


struct point {
    int x;
    int y;
};
struct byx {
    constexpr bool operator()(point const& l, point const& r) const { return l.x > r.x; }
};

// Only the comparison is used, there's no need for `operator==`
void compare()
{
    cobskiplist<point, 8, byx> l;
    CHECK(l.insert({ 1, 10 }).second);
    CHECK(l.insert({ 3, 30 }).second);
    CHECK(l.insert({ 2, 20 }).second);
    CHECK(!l.insert({ 2, 99 }).second);
    CHECK(l.insert({ 2, 99 }).first->y == 20);
    int exp = 3;
    for (auto const& p : l) {
        CHECK(p.x == exp && p.y == exp * 10);
        --exp;
    }
    CHECK(l.count({ 3, 0 }) == 1);
    CHECK(l.find({ 1, 0 })->y == 10);
    CHECK(l.erase({ 2, 0 }) == 1);
    CHECK(l.size() == 2);
}


// Random operations, compared to what `std::set` does
void model()
{
    std::mt19937 rng(20231114);
    for (int round = 0; round < 50; ++round) {
        cobskiplist<int, 64> l;
        std::set<int>        ref;
        for (int op = 0; op < 2000; ++op) {
            const int v = static_cast<int>(rng() % 100);
            switch (rng() % 6) {
            case 0:
            case 1:
            case 2: {
                const bool in  = ref.count(v) > 0;
                const bool exp = !in && (ref.size() < 64);
                auto       r   = l.insert(v);
                CHECK(r.second == exp);
                if (exp) ref.insert(v);
                if (in || exp) {
                    CHECK(*r.first == v);
                }
                else {
                    CHECK(r.first == l.end());
                }
                break;
            }
            case 3:
                CHECK(l.erase(v) == ref.erase(v));
                break;
            case 4: {
                CHECK(l.count(v) == ref.count(v));
                auto f = l.find(v);
                CHECK((f != l.end()) == (ref.count(v) > 0));
                if (f != l.end()) CHECK(*f == v);
                auto lb  = l.lower_bound(v);
                auto rlb = ref.lower_bound(v);
                CHECK((lb == l.end()) == (rlb == ref.end()));
                if (rlb != ref.end()) CHECK(*lb == *rlb);
                break;
            }
            case 5: {
                auto lnk = l.lbegin();
                auto rit = ref.begin();
                for (unsigned i = rng() % 5; (i > 0) && (lnk != l.lend()); --i) {
                    lnk = l.fwd(lnk);
                    ++rit;
                }
                if (lnk != l.lend()) {
                    CHECK(l.contains(lnk));
                    const auto nxt = l.fwd(lnk);
                    CHECK(l.erase(lnk) == nxt);
                    ref.erase(rit);
                }
                break;
            }
            }
            CHECK(l.size() == ref.size());
            CHECK(l.empty() == ref.empty());
            CHECK(same(l, ref));
        }
    }
}

static_assert(std::forward_iterator<cobskiplist<int, 8>::CI>);
static_assert(std::is_same_v<decltype(*std::declval<cobskiplist<int, 8>&>().begin()), int const&>);

void stditers()
{
    cobskiplist<int, 8> s;
    for (int i : { 5, 1, 3 }) { s.insert(i); }
    CHECK(*std::ranges::find(s, 3) == 3);
    CHECK(std::ranges::find(s, 9) == s.end());
    auto b = s.begin();
    auto c = b++;
    CHECK(c == s.begin());
    CHECK(*b == 3);
    CHECK(s.begin() == s.cbegin());
}


struct byv {
    bool operator()(nodefault const& a, nodefault const& b) const { return a.v < b.v; }
};

void objects()
{
    const int base = counted::alive;
    struct cless {
        bool operator()(counted const& a, counted const& b) const { return a.v < b.v; }
    };
    {
        cobskiplist<counted, 16, cless> s;
        for (int i = 0; i < 6; ++i) { CHECK(s.insert(counted(i)).second); }
        CHECK(counted::alive == base + 6);
        CHECK(!s.insert(counted(3)).second);
        CHECK(counted::alive == base + 6);
        CHECK(s.erase(counted(3)) == 1);
        CHECK(counted::alive == base + 5);
        CHECK(s.emplace(10).second);
        CHECK(counted::alive == base + 6);
        s.erase(s.lbegin());
        CHECK(counted::alive == base + 5);
        s.clear();
        CHECK(counted::alive == base);
        s.insert(counted(1));
    }
    CHECK(counted::alive == base);

    {
        std::string a(40, 'a'), b(41, 'b'), c(42, 'c');
        cobskiplist<std::string, 16> s;
        s.insert(c); s.insert(a); s.insert(b); s.insert(a);
        CHECK((std::vector<std::string>(s.begin(), s.end()) == std::vector<std::string>{ a, b, c }));
        CHECK(s.erase(b) == 1);
        CHECK(s.count(a) == 1);
        CHECK(*s.find(c) == c);
    }

    {
        cobskiplist<nodefault, 16, byv> s;
        CHECK(s.emplace(3).second);
        CHECK(s.emplace(1).second);
        CHECK(!s.emplace(3).second);
        CHECK(s.erase(nodefault(1)) == 1);
        CHECK(s.size() == 1);
    }

    {
        cobskiplist<int, 16> s;
        s.insert(1);
        s.insert(2);
        auto lk = s.lfront();
        auto it = s.begin();
        s.erase(lk);
        CHECK(!s.live(lk));
        CHECK(!s.contains(lk));
        CHECK(s.erase(lk) == s.lend());
        (void)*it; // does not crash
    }
}


void upper()
{
    cobskiplist<int, 64> s;
    std::set<int> ref;
    std::mt19937 gen(3);
    for (int i = 0; i < 40; ++i) {
        int v = static_cast<int>(gen() % 100);
        s.insert(v);
        ref.insert(v);
    }
    for (int v = -1; v <= 101; ++v) {
        auto u = s.upper_bound(v);
        auto r = ref.upper_bound(v);
        CHECK((u == s.end()) == (r == ref.end()));
        if (r != ref.end()) { CHECK(*u == *r); }
        auto const& cs = s;
        CHECK((cs.upper_bound(v) == cs.end()) == (r == ref.end()));
    }
    CHECK(s.size() == ref.size());

    // deterministic by default: same structure every time
    cobskiplist<int, 64> a, b;
    for (int i = 0; i < 30; ++i) { a.insert(i * 7 % 30); b.insert(i * 7 % 30); }
    std::ostringstream oa, ob;
    a.printme(oa);
    b.printme(ob);
    CHECK(oa.str() == ob.str());

    // a generator of your own
    struct always_zero { unsigned operator()() { return 0; } };
    cobskiplist<int, 16, cobhlp::less<int>, always_zero> z;
    for (int i = 0; i < 10; ++i) { z.insert(i); }
    CHECK(z.size() == 10 && z.count(4) == 1);
}


int main()
{
    upper();
    objects();
    stditers();
    basic();
    order();
    recycle();
    compare();
    model();

    return 0;
}
