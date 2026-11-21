#include "cobmap.hpp"
#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

static_assert(std::forward_iterator<cobmap<int, int, 8>::CI>);
static_assert(std::is_same_v<decltype(std::declval<cobmap<int, int, 8>&>().begin()->key), int const>);

void basics()
{
    cobmap<std::string, int, 8> m;
    CHECK(m.empty() && !m.full() && m.size() == 0 && m.max_size() == 8);

    auto b = m.insert("b", 2);
    CHECK(b.second && b.first != m.nil);
    CHECK(m.emplace("a", 1).second);
    CHECK(m.insert("c", 3).second);
    CHECK(m.size() == 3 && !m.empty());

    // the key is there: nothing changes, the link is that of what is there
    auto again = m.insert("b", 20);
    CHECK(!again.second && again.first == b.first);
    CHECK(m.find<std::optional>("b") == 2);

    CHECK(m.contains("a") && !m.contains("z") && m.count("a") == 1 && m.count("z") == 0);
    CHECK(!m.find<std::optional>("z").has_value());
    CHECK(m.find<perhaps>("c").value_or(0) == 3);
    CHECK(m.where("b") == b.first && m.where("z") == m.nil);

    // in the order of the keys
    std::string keys;
    int         sum = 0;
    for (auto const& e : m) {
        keys += e.key;
        sum += e.value;
    }
    CHECK(keys == "abc" && sum == 6);
    keys.clear();
    m.for_each([&](std::string const& k, int& v) {
        keys += k;
        v *= 10;
    });
    CHECK(keys == "abc" && m.find<std::optional>("c") == 30);
    cobmap<std::string, int, 8> const& cm = m;
    sum = 0;
    cm.for_each([&](std::string const&, int const& v) { sum += v; });
    CHECK(sum == 60);

    // changing a value
    CHECK(m.with("a", [](int& v) { v = 7; }));
    CHECK(!m.with("z", [](int&) { CHECK(false); }));
    int seen = 0;
    CHECK(cm.with("a", [&](int const& v) { seen = v; }) && seen == 7);
    CHECK(m.with(b.first, [](int& v) { ++v; }) && m.find<std::optional>("b") == 21);
    CHECK(cm.with(b.first, [&](int const& v) { seen = v; }) && seen == 21);

    auto ins = m.insert_or_assign("d", 4);
    CHECK(ins.second && m.size() == 4);
    auto asg = m.insert_or_assign("d", 5);
    CHECK(!asg.second && asg.first == ins.first && m.size() == 4 && m.find<std::optional>("d") == 5);

    // by link
    CHECK(m.key<std::optional>(m.lbegin()) == "a");
    CHECK(m.value<std::optional>(m.lbegin()) == 7);
    CHECK(!m.key<std::optional>(m.lend()).has_value() && !m.value<std::optional>(m.nil).has_value());
    CHECK(m.key<std::optional>(m.fwd(m.lbegin())) == "b");
    CHECK(m.key<std::optional>(m.lower_bound("bb")) == "c");
    CHECK(m.key<std::optional>(m.lower_bound("c")) == "c");
    CHECK(m.key<std::optional>(m.upper_bound("c")) == "d");
    CHECK(m.lower_bound("x") == m.lend() && m.upper_bound("d") == m.lend());

    // iterators
    CHECK(m.find("c")->value == 30 && m.find("c")->key == "c");
    CHECK(m.find("z") == m.end());
    CHECK(m.contains(m.find("c")) && !m.contains(m.end()));

    // erasing
    CHECK(m.erase("z") == 0 && m.erase("b") == 1 && m.size() == 3);
    CHECK(!m.live(b.first) && !m.contains(b.first));
    CHECK(!m.with(b.first, [](int&) { CHECK(false); }));   // a stale link is harmless
    CHECK(m.erase(b.first) == m.nil && m.size() == 3);
    CHECK(m.key<std::optional>(m.erase(m.lbegin())) == "c");
    auto it = m.erase(m.find("c"));
    CHECK(it != m.end() && it->key == "d");
    m.clear();
    CHECK(m.empty() && m.begin() == m.end() && m.lbegin() == m.lend());
    CHECK(m.insert("q", 1).second);
}

void full()
{
    cobmap<int, int, 4> m;
    for (int i = 0; i < 4; ++i) {
        CHECK(m.insert(i, i).second);
    }
    CHECK(m.full());
    auto none = m.insert(9, 9);
    CHECK(!none.second && none.first == m.nil);
    none = m.insert_or_assign(9, 9);
    CHECK(!none.second && none.first == m.nil && !m.contains(9));
    CHECK(!m.insert_or_assign(2, 22).second && m.find<std::optional>(2) == 22);
    CHECK(m.erase(0) == 1 && m.insert(9, 9).second && m.full());
}

struct descending {
    bool operator()(int x, int y) const { return x > y; }
};

void anytype()
{
    // no default constructor
    cobmap<int, nodefault, 4> nd;
    CHECK(nd.emplace(1, 5).second && nd.insert(2, nodefault{ 6 }).second);
    int v = 0;
    CHECK(nd.with(2, [&](nodefault& x) { v = x.v; }) && v == 6);

    // can't be copied
    cobmap<int, std::unique_ptr<int>, 4> up;
    CHECK(up.emplace(2, new int(20)).second);
    CHECK(up.insert(1, std::make_unique<int>(10)).second);
    CHECK(!up.insert_or_assign(1, std::make_unique<int>(11)).second);
    int sum = 0;
    up.for_each([&](int const& k, std::unique_ptr<int>& p) { sum += k * *p; });
    CHECK(sum == 11 + 40);

    // made when put in, destroyed when taken out
    counted::alive = 0;
    {
        cobmap<int, counted, 8> m;
        CHECK(counted::alive == 0);
        for (int i = 0; i < 8; ++i) {
            CHECK(m.emplace(i, i).second);
        }
        CHECK(counted::alive == 8 && !m.emplace(8, 8).second && counted::alive == 8);
        CHECK(!m.emplace(3, 33).second && counted::alive == 8);     // nothing is made for a key that is there
        CHECK(m.erase(3) == 1 && counted::alive == 7);

        // a copy is a map of its own
        cobmap<int, counted, 8> c = m;
        CHECK(counted::alive == 14 && c.size() == 7 && c.contains(4) && !c.contains(3));
        c.with(4, [](counted& x) { x.v = 44; });
        CHECK(m.find<perhaps>(4).value_or(counted{ -1 }).v == 4);
        CHECK(c.erase(5) == 1 && m.contains(5));
        m = c;
        CHECK(counted::alive == 12 && !m.contains(5) && m.size() == 6);
        m.clear();
        CHECK(counted::alive == 6);
    }
    CHECK(counted::alive == 0);

    cobmap<int, int, 8, descending> d;
    for (int i : { 3, 1, 2 }) {
        d.insert(i, i);
    }
    std::vector<int> keys;
    for (auto const& e : d) {
        keys.push_back(e.key);
    }
    CHECK((keys == std::vector<int>{ 3, 2, 1 }));
    CHECK(d.key<std::optional>(d.lower_bound(2)) == 2 && d.key<std::optional>(d.upper_bound(2)) == 1);
}

void against_std()
{
    std::mt19937 rng(20241116);
    for (int round = 0; round < 20; ++round) {
        cobmap<int, int, 64> m;
        std::map<int, int>   model;
        for (int step = 0; step < 3000; ++step) {
            const int k = static_cast<int>(rng() % 100);
            const int v = static_cast<int>(rng() % 1000);
            switch (rng() % 5) {
            case 0:
            case 1: {
                const bool room = model.size() < 64;
                const bool isin = model.count(k) > 0;
                CHECK(m.insert(k, v).second == (room && !isin));
                if (room) {
                    model.insert({ k, v });
                }
                break;
            }
            case 2: {
                const bool isin = model.count(k) > 0;
                const auto r    = m.insert_or_assign(k, v);
                CHECK(r.second == (!isin && model.size() < 64));
                if (isin || model.size() < 64) {
                    model[k] = v;
                    CHECK(r.first != m.nil);
                }
                break;
            }
            case 3: CHECK(m.erase(k) == model.erase(k)); break;
            default: {
                auto it = model.lower_bound(k);
                auto l  = m.lower_bound(k);
                CHECK((it == model.end()) ? (l == m.lend()) : (m.key<std::optional>(l) == it->first));
                break;
            }
            }
            CHECK(m.size() == model.size());
            const auto found = m.find<std::optional>(k);
            CHECK(found.has_value() == (model.count(k) > 0));
            if (found) {
                CHECK(*found == model[k]);
            }
        }
        auto it = model.begin();
        for (auto const& e : m) {
            CHECK(it != model.end() && e.key == it->first && e.value == it->second);
            ++it;
        }
        CHECK(it == model.end());
    }
}

int main()
{
    basics();
    full();
    anytype();
    against_std();
    return 0;
}
