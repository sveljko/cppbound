#include "cobpool.hpp"
#include "cobtesthelp.hpp"

#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
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

int main()
{
    basics();
    rounds();
    objects();
    model();

    return 0;
}
