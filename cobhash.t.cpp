#include "cobhash.hpp"
#include "cobtesthelp.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

struct zero {
    std::size_t operator()(int) const { return 0; }
};
struct mod3 {
    std::size_t operator()(int k) const { return static_cast<std::size_t>(k) % 3; }
};

void basics()
{
    cobhash<int, int, 8> h;
    CHECK(h.empty() && h.max_size() == 8);
    CHECK(h.insert(1, 10) && h.insert(2, 20));
    CHECK(!h.insert(1, 11));
    CHECK(h.contains(1) && !h.contains(3));
    CHECK(h.template find<std::optional>(1) == 10 && !h.find<std::optional>(3).has_value());
    CHECK(h.insert_or_assign(1, 12) && h.find<std::optional>(1) == 12);
    CHECK(h.insert_or_assign(3, 30) && h.size() == 3);
    CHECK(h.with(2, [](int& v) { v = 21; }));
    CHECK(!h.with(9, [](int&) {}));
    cobhash<int, int, 8> const& ch = h;
    int                         seen = 0;
    CHECK(ch.with(2, [&](int const& v) { seen = v; }) && seen == 21);
    int sum = 0;
    h.for_each([&](int const& k, int& v) { sum += k + v; });
    CHECK(sum == (1 + 12) + (2 + 21) + (3 + 30));
    CHECK(h.erase(2) && !h.erase(2) && h.size() == 2 && !h.contains(2));
    h.clear();
    CHECK(h.empty());

    cobhash<std::string, std::string, 4> s;
    CHECK(s.insert(std::string(40, 'k'), std::string(50, 'v')));
    CHECK(s.find<std::optional>(std::string(40, 'k'))->size() == 50);
    CHECK(s.erase(std::string(40, 'k')) && s.empty());
}

void objects()
{
    const int base = counted::alive;
    {
        cobhash<int, counted, 8, zero> h;
        for (int i = 0; i < 6; ++i) { CHECK(h.insert(i, counted(i))); }
        CHECK(counted::alive == base + 6);
        // erasing shifts entries back: nothing is lost or duplicated
        CHECK(h.erase(1) && h.erase(4));
        CHECK(counted::alive == base + 4);
        for (int i : { 0, 2, 3, 5 }) {
            CHECK(h.template find<std::optional>(i)->v == i);
        }
        cobhash<int, counted, 8, zero> c = h;
        CHECK(counted::alive == base + 8);
        cobhash<int, counted, 8, zero> d;
        d.insert(7, counted(7));
        d = h;
        CHECK(counted::alive == base + 12);
        auto& self = d;
        d          = self;
        CHECK(counted::alive == base + 12 && d.size() == 4);
        c.clear();
        CHECK(counted::alive == base + 8);
    }
    CHECK(counted::alive == base);
}

template <class H> void bad_hash()
{
    cobhash<int, int, 7, H> h;
    for (int i = 0; i < 7; ++i) { CHECK(h.insert(i, i * 10)); }
    CHECK(h.full() && !h.insert(99, 0));
    CHECK(!h.insert_or_assign(99, 0));
    CHECK(h.insert_or_assign(3, 1));
    CHECK(!h.contains(99)); // terminates
    h.insert_or_assign(3, 30);
    for (int i = 0; i < 7; ++i) { CHECK(h.template find<std::optional>(i) == i * 10); }

    std::mt19937 gen(1);
    for (int round = 0; round < 50; ++round) {
        std::vector<int> keys{ 0, 1, 2, 3, 4, 5, 6 };
        std::shuffle(keys.begin(), keys.end(), gen);
        for (std::size_t i = 0; i < keys.size(); ++i) {
            CHECK(h.erase(keys[i]));
            for (std::size_t j = i + 1; j < keys.size(); ++j) {
                CHECK(h.template find<std::optional>(keys[j]) == keys[j] * 10);
            }
            CHECK(!h.contains(keys[i]));
        }
        CHECK(h.empty());
        std::shuffle(keys.begin(), keys.end(), gen);
        for (int k : keys) { CHECK(h.insert(k, k * 10)); }
        CHECK(h.full());
    }
}

template <class H> void model()
{
    cobhash<int, int, 16, H>  h;
    std::unordered_map<int, int> ref;
    std::mt19937              gen(4);
    for (int op = 0; op < 20000; ++op) {
        const int k = static_cast<int>(gen() % 40);
        const int v = static_cast<int>(gen() % 1000);
        switch (gen() % 3) {
        case 0: {
            const bool ok = h.insert(k, v);
            const bool exp = (ref.count(k) == 0) && (ref.size() < 16);
            CHECK(ok == exp);
            if (exp) { ref[k] = v; }
            break;
        }
        case 1: {
            const bool ok = h.insert_or_assign(k, v);
            const bool exp = (ref.count(k) == 1) || (ref.size() < 16);
            CHECK(ok == exp);
            if (exp) { ref[k] = v; }
            break;
        }
        default:
            CHECK(h.erase(k) == (ref.erase(k) == 1));
            break;
        }
        CHECK(h.size() == ref.size());
        for (int i = 0; i < 40; ++i) {
            auto it = ref.find(i);
            CHECK(h.contains(i) == (it != ref.end()));
            auto f = h.template find<std::optional>(i);
            CHECK(f.has_value() == (it != ref.end()));
            if (f) { CHECK(*f == it->second); }
        }
    }
}

int main()
{
    basics();
    objects();
    bad_hash<zero>();
    bad_hash<mod3>();
    model<std::hash<int>>();
    model<mod3>();

    return 0;
}
