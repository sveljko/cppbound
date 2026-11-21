#include "atbpool.hpp"
#include "cobpool.hpp"
#include "cobtesthelp.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

void basics()
{
    cobpool<int, 4> p;
    CHECK(p.empty() && !p.full() && p.size() == 0 && p.max_size() == 4);
    auto a = p.make(10);
    auto b = p.make(20);
    CHECK(a != p.nil && b != p.nil && a != b);
    CHECK(p.live(a) && p.live(b) && !p.live(p.nil));
    CHECK(p.get(a) == 10 && p.get(b) == 20 && p.size() == 2);
    CHECK(p.with(a, [](int& x) { x = 11; }));
    CHECK(p.get(a) == 11);
    cobpool<int, 4> const& cp = p;
    int                    seen = 0;
    CHECK(cp.with(b, [&](int const& x) { seen = x; }) && seen == 20);

    // stale
    CHECK(p.drop(a));
    CHECK(!p.drop(a));
    CHECK(!p.live(a));
    int calls = 0;
    CHECK(!p.with(a, [&](int&) { ++calls; }));
    CHECK(calls == 0 && p.get(a) == 0);
    CHECK(!p.drop(p.nil));

    // full
    cobpool<int, 3> f;
    CHECK(f.make(1) != f.nil && f.make(2) != f.nil && f.make(3) != f.nil);
    CHECK(f.full() && f.make(4) == f.nil);
    int sum = 0;
    f.for_each([&](int& x) { sum += x; });
    CHECK(sum == 6);
    f.clear();
    CHECK(f.empty());
}

void rounds()
{
    cobpool<int, 8> p;
    std::mt19937    gen(5);
    for (int round = 0; round < 50; ++round) {
        std::vector<cobpool<int, 8>::handle> hs;
        for (int i = 0; i < 8; ++i) {
            auto h = p.make(i);
            CHECK(h != p.nil);
            hs.push_back(h);
        }
        CHECK(p.full() && p.make(0) == p.nil);
        std::shuffle(hs.begin(), hs.end(), gen);
        for (std::size_t i = 0; i < hs.size(); ++i) {
            CHECK(p.drop(hs[i]));
            CHECK(p.size() == 7 - i);
        }
    }
}

void objects()
{
    const int base = counted::alive;
    {
        cobpool<counted, 4> p;
        auto                a = p.make(1);
        auto                b = p.make(2);
        p.make(3);
        CHECK(counted::alive == base + 3);
        p.drop(b);
        CHECK(counted::alive == base + 2);
        // handles stay valid while others come and go
        auto c = p.make(4);
        p.drop(c);
        p.make(5);
        CHECK(p.get(a).v == 1);
        CHECK(counted::alive == base + 3);

        cobpool<counted, 4> q = p;
        CHECK(counted::alive == base + 6);
        CHECK(q.get(a).v == 1);
        cobpool<counted, 4> r;
        r.make(9);
        r = p;
        CHECK(counted::alive == base + 9);
        for (int h = 0; h < 4; ++h) {
            cobpool<counted, 4>::handle hh;
            hh.be(h);
            CHECK(p.live(hh) == q.live(hh) && p.live(hh) == r.live(hh));
            if (p.live(hh)) { CHECK(p.get(hh).v == q.get(hh).v && p.get(hh).v == r.get(hh).v); }
        }
        auto& self = r;
        r          = self;
        CHECK(counted::alive == base + 9);
    }
    CHECK(counted::alive == base);

    cobpool<nodefault, 2> n;
    auto                  h = n.make(4);
    int                   v = 0;
    CHECK(n.with(h, [&](nodefault& x) { v = x.v; }) && v == 4);
    n.for_each([&](nodefault& x) { v += x.v; });
    CHECK(v == 8);

    cobpool<std::string, 2> s;
    auto                    hs = s.make(50, 'x');
    CHECK(s.get(hs).size() == 50);
}

void model()
{
    using pool = cobpool<int, 16>;
    pool                 p;
    std::map<int, int>   ref;
    std::mt19937         gen(11);
    for (int op = 0; op < 5000; ++op) {
        if ((gen() % 2 == 0) && (ref.size() < 16)) {
            const int v = static_cast<int>(gen() % 1000);
            auto      h = p.make(v);
            CHECK(h != p.nil);
            CHECK(ref.find(h.get()) == ref.end());
            ref[h.get()] = v;
        }
        else if (!ref.empty()) {
            auto it = ref.begin();
            std::advance(it, gen() % ref.size());
            pool::handle h;
            CHECK(h.be(it->first));
            CHECK(p.drop(h));
            ref.erase(it);
        }
        CHECK(p.size() == ref.size());
        for (int i = 0; i < 16; ++i) {
            pool::handle h;
            h.be(i);
            CHECK(p.live(h) == (ref.count(i) == 1));
            if (ref.count(i)) { CHECK(p.get(h) == ref[i]); }
        }
    }
}

// With checked handles, the handle of a dropped object stays the handle of
// nothing, also when a new object takes the slot
struct four {};     // the tag of the dimension, which is 4

template <class P> void checked()
{
    P                  p;
    typename P::handle none;
    CHECK(none == p.nil && !p.live(none) && !p.drop(none) && p.get(none).empty());

    const auto a = p.make("a");
    const auto b = p.make("b");
    CHECK(a != p.nil && a != b && p.live(a) && p.get(a) == "a" && p.size() == 2);
    CHECK(p.drop(a) && !p.live(a) && !p.drop(a) && p.get(a).empty());
    const auto c = p.make("c");     // takes the slot that `a` was in
    CHECK(p.live(c) && p.get(c) == "c" && a != c);
    CHECK(!p.live(a) && p.get(a).empty() && !p.drop(a) && p.live(c));
    CHECK(!p.with(a, [](std::string&) { CHECK(false); }) && p.with(c, [](std::string& s) { s += "!"; }));
    P const& cp = p;
    CHECK(!cp.with(a, [](std::string const&) { CHECK(false); }) && cp.get(c) == "c!");

    // the handles are good for a copy, as far as they were good
    P q = p;
    CHECK(q.get(c) == "c!" && q.get(b) == "b" && !q.live(a) && q.size() == 2);

    // clearing drops them all
    p.clear();
    CHECK(p.empty() && !p.live(b) && !p.live(c) && q.live(b) && q.live(c));
    std::vector<typename P::handle> all;
    for (auto h = p.make("x"); h != p.nil; h = p.make("x")) {
        CHECK(!p.live(b) && !p.live(c));
        all.push_back(h);
    }
    CHECK(p.full() && all.size() == 4 && p.size() == p.max_size());

    // each slot, reused over and over: only the latest handle is good
    for (int round = 0; round < 100; ++round) {
        for (auto& h : all) {
            const auto was = h;
            CHECK(p.drop(h));
            h = p.make(std::to_string(round));
            CHECK(h != p.nil && h != was && !p.live(was) && p.get(h) == std::to_string(round));
        }
    }
    std::size_t n = 0;
    p.for_each([&](std::string const&) { ++n; });
    CHECK(n == 4);
}

// The pool that is good at going over its objects: which ones there are is
// kept as a bit each, and they can be walked by their handles
struct many {};     // the tag of the dimension, which is 200

template <class P>
concept walkable = requires(P p) { p.next(p.first()); };

template <class P> void hive(unsigned capacity)
{
    P p;
    CHECK(p.first() == p.nil && p.next(p.nil) == p.nil && p.empty() && p.max_size() == capacity);

    std::mt19937                                      rng(20241125);
    std::vector<std::pair<typename P::handle, int>>   model;      // in no order
    auto walk = [&] {
        // in the order of the slots: each object once, and nothing else
        std::vector<int> seen;
        std::size_t      steps = 0;
        for (auto h = p.first(); h != p.nil; h = p.next(h)) {
            CHECK(p.live(h) && (++steps <= model.size()));
            seen.push_back(p.get(h));
        }
        std::vector<int> each;
        p.for_each([&](int const& x) { each.push_back(x); });
        CHECK(seen == each && seen.size() == model.size());
        std::vector<int> want;
        for (auto const& m : model) {
            want.push_back(m.second);
        }
        std::sort(seen.begin(), seen.end());
        std::sort(want.begin(), want.end());
        CHECK(seen == want);
    };
    for (int step = 0; step < 5000; ++step) {
        // mostly growing, then mostly shrinking, so that it gets full, and sparse
        const bool grow = (step / 500) % 2 == 0;
        if (model.empty() || (rng() % 4 < (grow ? 3u : 1u))) {
            const auto h = p.make(step);
            CHECK((h != p.nil) == (model.size() < capacity));
            if (h != p.nil) {
                model.push_back({ h, step });
            }
        }
        else {
            const std::size_t i = rng() % model.size();
            CHECK(p.get(model[i].first) == model[i].second && p.drop(model[i].first) && !p.drop(model[i].first));
            model.erase(model.begin() + static_cast<std::ptrdiff_t>(i));
        }
        CHECK(p.size() == model.size());
        if (step % 50 == 0) {
            walk();
        }
    }
    walk();

    // a copy is walked the same
    P copy = p;
    std::size_t n = 0;
    for (auto h = copy.first(); h != copy.nil; h = copy.next(h)) {
        CHECK(copy.get(h) == p.get(h));
        ++n;
    }
    CHECK(n == model.size());

    // dropping while walking: what follows is asked for first
    for (auto h = p.first(); h != p.nil;) {
        const auto after = p.next(h);
        CHECK(p.drop(h));
        h = after;
    }
    CHECK(p.empty() && p.first() == p.nil && copy.size() == model.size());
    p.clear();
    copy.clear();
    CHECK(copy.empty() && copy.first() == copy.nil);

    // the last slot, and the one before a whole word of none
    std::vector<typename P::handle> all;
    for (auto h = p.make(1); h != p.nil; h = p.make(1)) {
        all.push_back(h);
    }
    CHECK(all.size() == capacity && p.full());
    for (std::size_t i = 1; i + 1 < all.size(); ++i) {
        CHECK(p.drop(all[i]));
    }
    // the two that are left, far apart, in whichever order their slots are
    const auto one = p.first();
    const auto two = p.next(one);
    CHECK(((one == all.front()) || (one == all.back())) && (p.size() == ((capacity < 2) ? 1u : 2u)));
    CHECK((capacity < 2) ? (two == p.nil) : (((two == all.front()) || (two == all.back())) && (two != one) && (p.next(two) == p.nil)));
}

int main()
{
    basics();
    rounds();
    objects();
    model();

    CHECK(atbdim<four>::set(4));
    checked<cobcheckedpool<std::string, 4>>();
    checked<atbcheckedpool<std::string, four>>();

    CHECK(atbdim<many>::set(200));
    hive<cobhive<int, 200>>(200);
    hive<cobhive<int, 64>>(64);
    hive<cobhive<int, 65>>(65);
    hive<cobhive<int, 1>>(1);
    hive<cobcheckedhive<int, 200>>(200);
    hive<atbhive<int, many>>(200);
    hive<atbcheckedhive<int, many>>(200);
    checked<cobcheckedhive<std::string, 4>>();
    checked<atbcheckedhive<std::string, four>>();
    // a bit for each slot, where the pool has a byte
    static_assert(sizeof(cobhive<char, 1024>) < sizeof(cobpool<char, 1024>) - 800);
    // the handle of what was dropped does not lead on, if the handles are checked
    cobcheckedhive<int, 4> ch;
    const auto             a = ch.make(1);
    const auto             b = ch.make(2);
    CHECK(ch.next(a) == b && ch.drop(a) && ch.next(a) == ch.nil && ch.first() == b);
    // the pool that is not a hive has no walking
    static_assert(!walkable<cobpool<int, 4>> && !walkable<cobcheckedpool<int, 4>>);
    static_assert(walkable<cobhive<int, 4>> && walkable<cobcheckedhive<int, 4>>);

    return 0;
}
