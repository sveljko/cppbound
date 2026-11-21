#include "atbtree.hpp"
#include "atbtreemap.hpp"
#include "cobtree.hpp"
#include "cobtreemap.hpp"

#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

struct sized {};    // the tag of the dimension, which is 200

static_assert(std::bidirectional_iterator<cobtree<int, 8>::CI>);
static_assert(std::bidirectional_iterator<atbtree<int, sized>::CI>);
static_assert(std::is_same_v<decltype(*std::declval<cobtree<int, 8>&>().begin()), int const&>);

template <class TREE> void basics()
{
    TREE t;
    CHECK(t.empty() && !t.full() && t.size() == 0 && t.height() == 0 && t.sound());
    CHECK(t.begin() == t.end() && t.lbegin() == t.lend() && t.lback() == t.nil);
    CHECK(t.erase(5) == 0 && t.find(5) == t.end() && t.count(5) == 0);

    for (int x : { 50, 30, 70, 20, 40, 60, 80 }) {
        auto r = t.insert(x);
        CHECK(r.second && *r.first == x);
    }
    auto again = t.insert(40);
    CHECK(!again.second && *again.first == 40 && t.size() == 7 && t.height() == 3 && t.sound());
    CHECK(t.emplace(45).second);

    std::vector<int> all(t.begin(), t.end());
    CHECK((all == std::vector<int>{ 20, 30, 40, 45, 50, 60, 70, 80 }));
    std::size_t links = 0;
    for (auto l = t.lback(); l != t.nil; l = t.bck(l)) {
        ++links;
    }
    CHECK(links == all.size());
    CHECK(std::equal(std::make_reverse_iterator(t.end()), std::make_reverse_iterator(t.begin()), all.rbegin(), all.rend()));
    auto it = t.end();
    --it;
    CHECK(*it == 80 && *--it == 70 && *it-- == 70 && *it == 60);

    CHECK(t.count(45) == 1 && *t.find(45) == 45 && t.find(46) == t.end());
    CHECK(*t.lower_bound(45) == 45 && *t.lower_bound(46) == 50 && *t.upper_bound(45) == 50);
    CHECK(t.lower_bound(81) == t.end() && t.upper_bound(80) == t.end() && *t.lower_bound(-5) == 20);

    // A link stays good while its element is in, whatever else is erased or inserted
    const auto l60 = t.lower_bound(60);
    const auto k   = t.fwd(t.fwd(t.fwd(t.fwd(t.fwd(t.lbegin())))));     // the link of 60
    CHECK(t.erase(50) == 1 && t.erase(20) == 1 && t.erase(80) == 1 && t.insert(55).second && t.sound());
    CHECK(*l60 == 60 && t.contains(l60) && t.live(k) && t.contains(k));

    // erasing by link and by iterator gives what follows
    auto after = t.erase(t.find(40));
    CHECK(after != t.end() && *after == 45 && t.sound());
    CHECK(t.erase(k) != t.nil && !t.live(k) && t.erase(k) == t.nil && !t.contains(l60) && t.sound());
    CHECK(*l60 == 0);       // the iterator of what was erased is harmless
    TREE other;
    other.insert(1);
    CHECK(t.erase(other.begin()) == t.end() && other.size() == 1 && t.erase(t.end()) == t.end());

    // a copy is a tree of its own
    TREE copy = t;
    CHECK(std::equal(copy.begin(), copy.end(), t.begin(), t.end()) && copy.sound());
    copy.clear();
    CHECK(copy.empty() && copy.sound() && !t.empty() && copy.insert(3).second);
    t = copy;
    CHECK(t.size() == 1 && *t.begin() == 3 && t.sound());
    typename TREE::CI none;
    CHECK(*none == 0 && none != t.begin());
}

// Whatever is done, in whatever order, the tree is balanced and holds what it should
template <class TREE> void against_std(unsigned capacity)
{
    std::mt19937 rng(20241120);
    for (int round = 0; round < 30; ++round) {
        TREE          t;
        std::set<int> model;
        // few values: mostly the same ones; many: mostly new ones
        const int range = (round % 3 == 0) ? 60 : ((round % 3 == 1) ? 250 : 5000);
        for (int step = 0; step < 4000; ++step) {
            const int      x  = static_cast<int>(rng() % range);
            const unsigned op = rng() % 10;
            if (op < 5) {
                const bool should = (model.size() < capacity) && !model.count(x);
                const auto r      = t.insert(x);
                CHECK(r.second == should);
                if (should) {
                    model.insert(x);
                }
                CHECK((r.first == t.end()) == !model.count(x));
            }
            else if (op < 8) {
                CHECK(t.erase(x) == model.erase(x));
            }
            else if (op == 8) {
                if (!model.empty()) {
                    auto       mit   = std::next(model.begin(), static_cast<std::ptrdiff_t>(rng() % model.size()));
                    const auto after = t.erase(t.find(*mit));
                    mit              = model.erase(mit);
                    CHECK((mit == model.end()) ? (after == t.end()) : (*after == *mit));
                }
            }
            else {
                const auto lb = model.lower_bound(x);
                const auto ub = model.upper_bound(x);
                CHECK((lb == model.end()) ? (t.lower_bound(x) == t.end()) : (*t.lower_bound(x) == *lb));
                CHECK((ub == model.end()) ? (t.upper_bound(x) == t.end()) : (*t.upper_bound(x) == *ub));
            }
            CHECK(t.size() == model.size() && t.count(x) == model.count(x));
            if (step % 16 == 0) {
                CHECK(t.sound());
                CHECK(std::equal(t.begin(), t.end(), model.begin(), model.end()));
                CHECK(std::equal(std::make_reverse_iterator(t.end()), std::make_reverse_iterator(t.begin()),
                                 model.rbegin(), model.rend()));
                // never higher than an AVL tree may be
                CHECK(model.empty() || (t.height() <= 1.4405 * std::log2(static_cast<double>(model.size()) + 2.0)));
            }
        }
        CHECK(t.sound());
    }
}

// What a list is the worst at
void ordered()
{
    cobtree<int, 1000> t;
    for (int i = 0; i < 1000; ++i) {
        CHECK(t.insert(i).second);
    }
    CHECK(t.sound() && t.full() && t.height() == 10 && !t.insert(1000).second && t.insert(1000).first == t.end());
    for (int i = 0; i < 1000; i += 2) {
        CHECK(t.erase(i) == 1);
    }
    CHECK(t.sound() && t.size() == 500 && t.height() <= 10);
    for (int i = 999; i > 0; i -= 2) {
        CHECK(t.erase(i) == 1);
    }
    CHECK(t.sound() && t.empty() && t.height() == 0);
    for (int i = 1000; i > 0; --i) {
        CHECK(t.insert(i).second);
    }
    CHECK(t.sound() && t.height() == 10);
}

struct descending {
    bool operator()(std::string const& x, std::string const& y) const { return x > y; }
};

void anytype()
{
    cobtree<std::string, 8, descending> d;
    for (auto s : { "b", "c", "a" }) {
        d.insert(s);
    }
    std::string all;
    for (auto const& s : d) {
        all += s;
    }
    CHECK(all == "cba");

    cobtree<nodefault, 4, decltype([](nodefault const& x, nodefault const& y) { return x.v < y.v; })> nd;
    CHECK(nd.emplace(2).second && nd.insert(nodefault{ 1 }).second && nd.size() == 2);

    counted::alive = 0;
    {
        cobtree<counted, 16, decltype([](counted const& x, counted const& y) { return x.v < y.v; })> t;
        CHECK(counted::alive == 0);
        for (int i = 0; i < 16; ++i) {
            CHECK(t.emplace(i * 7 % 16).second);
        }
        CHECK(counted::alive == 16 && !t.emplace(99).second && counted::alive == 16);
        CHECK(t.erase(counted{ 5 }) == 1);
        CHECK(counted::alive == 15);
        auto copy = t;
        CHECK(counted::alive == 30 && copy.sound());
        t.clear();
        CHECK(counted::alive == 15);
    }
    CHECK(counted::alive == 0);
}

// The map on the tree is the same map
template <class MAP> void maps(unsigned capacity)
{
    MAP m;
    CHECK(m.insert("b", 2).second && m.emplace("a", 1).second && !m.insert("a", 5).second);
    CHECK(m.template find<std::optional>("a") == 1 && !m.template find<std::optional>("z").has_value() && m.contains("b"));
    CHECK(m.with("a", [](int& v) { v = 7; }) && m.template find<perhaps>("a").value_or(0) == 7);
    CHECK(!m.insert_or_assign("b", 20).second && m.insert_or_assign("c", 3).second && m.find("c")->value == 3);
    CHECK(m.template key<std::optional>(m.lbegin()) == "a" && m.template key<std::optional>(m.upper_bound("a")) == "b");
    std::string keys;
    m.for_each([&](std::string const& k, int&) { keys += k; });
    CHECK(keys == "abc" && m.erase("b") == 1 && m.erase(m.lbegin()) != m.nil && m.size() == 1);
    m.clear();

    std::mt19937               rng(20241121);
    std::map<std::string, int> model;
    for (int step = 0; step < 6000; ++step) {
        const std::string k = std::to_string(rng() % 300);
        const int         v = static_cast<int>(rng() % 1000);
        switch (rng() % 3) {
        case 0: {
            const bool should = (model.size() < capacity) && !model.count(k);
            CHECK(m.insert(k, v).second == should);
            if (should) {
                model[k] = v;
            }
            break;
        }
        case 1: CHECK(m.erase(k) == model.erase(k)); break;
        default:
            CHECK(m.with(k, [&](int& x) { x = v; }) == (model.count(k) > 0));
            if (model.count(k)) {
                model[k] = v;
            }
        }
        CHECK(m.size() == model.size() && m.template find<std::optional>(k).has_value() == (model.count(k) > 0));
    }
    auto it = model.begin();
    for (auto const& e : m) {
        CHECK(it != model.end() && e.key == it->first && e.value == it->second);
        ++it;
    }
    CHECK(it == model.end());
}

int main()
{
    CHECK(atbdim<sized>::set(200));

    basics<cobtree<int, 16>>();
    basics<atbtree<int, sized>>();
    against_std<cobtree<int, 200>>(200);
    against_std<atbtree<int, sized>>(200);
    ordered();
    anytype();
    maps<cobtreemap<std::string, int, 200>>(200);
    maps<atbtreemap<std::string, int, sized>>(200);

    cobtreemap<int, std::unique_ptr<int>, 4> up;
    CHECK(up.emplace(1, new int(5)).second && up.erase(1) == 1 && up.empty());

    struct unset {};
    atbtree<int, unset> none;
    CHECK(!none.insert(1).second && none.empty() && none.full() && none.sound() && none.erase(1) == 0);
    return 0;
}
