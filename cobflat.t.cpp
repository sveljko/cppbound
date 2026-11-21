#include "atbflatmap.hpp"
#include "atbflatset.hpp"
#include "cobflatmap.hpp"
#include "cobflatset.hpp"

#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

struct sized {};    // the tag of the dimension, which is 40

static_assert(std::random_access_iterator<cobflatset<int, 8>::CI>);
static_assert(std::ranges::random_access_range<cobflatset<int, 8> const>);
static_assert(std::is_same_v<decltype(*std::declval<cobflatset<int, 8>&>().begin()), int const&>);
static_assert(std::is_same_v<decltype(std::declval<cobflatmap<int, int, 8>&>().begin()->value), int>);
static_assert(std::is_const_v<std::remove_reference_t<decltype(*std::declval<cobflatmap<int, int, 8>&>().begin())>>);
// Nothing but the elements and their number
static_assert(sizeof(cobflatset<int, 8>) == 8 * sizeof(int) + sizeof(unsigned));
// The index is that of the bound, so it is within the capacity
static_assert(std::is_same_v<cobflatset<int, 8>::index, cobint<0, 7>>);
static_assert(std::is_same_v<atbflatset<int, sized>::index, atbidx<sized>>);

template <class S> void sets(unsigned capacity)
{
    S s;
    CHECK(s.empty() && !s.full() && s.size() == 0 && s.max_size() == capacity && s.begin() == s.end());
    CHECK(s.erase(5) == 0 && s.find(5) == s.end() && !s.contains(5) && s.count(5) == 0);
    CHECK(!s.template front<std::optional>().has_value() && !s.template index_of<std::optional>(5).has_value());

    for (int x : { 50, 30, 70, 20, 40 }) {
        const auto r = s.insert(x);
        CHECK(r.second && *r.first == x);
    }
    const auto again = s.insert(40);
    CHECK(!again.second && *again.first == 40 && s.size() == 5 && s.emplace(45).second);
    CHECK(std::vector<int>(s.begin(), s.end()) == (std::vector<int>{ 20, 30, 40, 45, 50, 70 }));
    CHECK(s.template front<std::optional>() == 20 && s.template back<perhaps>().value_or(0) == 70);

    CHECK(s.contains(45) && s.count(45) == 1 && *s.find(45) == 45 && s.find(46) == s.end());
    CHECK(*s.lower_bound(45) == 45 && *s.lower_bound(46) == 50 && *s.upper_bound(45) == 50);
    CHECK(s.lower_bound(71) == s.end() && s.upper_bound(70) == s.end() && s.lower_bound(-1) == s.begin());
    CHECK(s.find(45) - s.begin() == 3 && s.end() - s.begin() == 6 && s.begin()[2] == 40);

    // by index: good until the set is changed, harmless after that
    const auto at = s.template index_of<std::optional>(45);
    CHECK(at.has_value() && s.get(*at) == 45 && s.template at<std::optional>(*at) == 45 && s.live(*at));
    int sum = 0;
    for (auto i : s.irange()) {
        sum += s.get(i);
    }
    CHECK(sum == 255);
    CHECK(s.erase(20) == 1 && s.get(*at) == 50);    // it is what is there now
    CHECK(s.erase(30) == 1 && s.erase(40) == 1 && s.erase(70) == 1 && !s.live(*at) && s.get(*at) == 0);
    CHECK(!s.template at<std::optional>(*at).has_value() && s.size() == 2);

    const auto after = s.erase(s.find(45));
    CHECK(after != s.end() && *after == 50 && s.erase(s.end()) == s.end() && s.size() == 1);
    sum = 0;
    s.for_each([&](int const& x) { sum += x; });
    CHECK(sum == 50);

    S copy = s;
    CHECK(copy == s && copy.insert(1).second && !(copy == s) && s.size() == 1);
    s.clear();
    CHECK(s.empty() && copy.size() == 2);

    // against the standard set
    std::mt19937  rng(20241123);
    std::set<int> model;
    for (int step = 0; step < 6000; ++step) {
        const int x = static_cast<int>(rng() % (2 * capacity));
        switch (rng() % 4) {
        case 0:
        case 1: {
            const bool should = (model.size() < capacity) && !model.count(x);
            const auto r      = s.insert(x);
            CHECK(r.second == should && ((r.first == s.end()) == (!should && !model.count(x))));
            if (should) model.insert(x);
            break;
        }
        case 2: CHECK(s.erase(x) == model.erase(x)); break;
        default: {
            const auto lb = model.lower_bound(x);
            const auto ub = model.upper_bound(x);
            CHECK((lb == model.end()) ? (s.lower_bound(x) == s.end()) : (*s.lower_bound(x) == *lb));
            CHECK((ub == model.end()) ? (s.upper_bound(x) == s.end()) : (*s.upper_bound(x) == *ub));
        }
        }
        CHECK(s.size() == model.size() && s.contains(x) == (model.count(x) > 0) && s.full() == (model.size() == capacity));
        CHECK(std::equal(s.begin(), s.end(), model.begin(), model.end()));
    }
    CHECK(std::ranges::is_sorted(s) && (std::ranges::binary_search(s, *model.begin()) || model.empty()));
}

template <class M> void maps(unsigned capacity)
{
    M m;
    CHECK(m.empty() && m.max_size() == capacity);
    CHECK(m.insert("b", 2).second && m.emplace("a", 1).second && m.insert("c", 3).second);
    const auto again = m.insert("b", 20);
    CHECK(!again.second && again.first->key == "b" && again.first->value == 2 && m.size() == 3);

    CHECK(m.contains("a") && !m.contains("z") && m.count("a") == 1);
    CHECK(m.template find<std::optional>("b") == 2 && !m.template find<std::optional>("z").has_value());
    CHECK(m.find("c")->value == 3 && m.find("z") == m.end());
    CHECK(m.lower_bound("b")->key == "b" && m.upper_bound("b")->key == "c" && m.lower_bound("zz") == m.end());

    std::string keys;
    for (auto const& e : m) {
        keys += e.key;
    }
    CHECK(keys == "abc");
    keys.clear();
    m.for_each([&](std::string const& k, int& v) {
        keys += k;
        v *= 10;
    });
    M const& cm = m;
    int      sum = 0;
    cm.for_each([&](std::string const&, int const& v) { sum += v; });
    CHECK(keys == "abc" && sum == 60);

    CHECK(m.with("a", [](int& v) { v = 7; }) && !m.with("z", [](int&) { CHECK(false); }));
    CHECK(cm.with("a", [&](int const& v) { sum = v; }) && sum == 7);
    auto ins = m.insert_or_assign("d", 4);
    CHECK(ins.second && ins.first->key == "d");
    ins = m.insert_or_assign("d", 5);
    CHECK(!ins.second && ins.first->value == 5 && m.size() == 4);

    // by index
    const auto b = m.template index_of<std::optional>("b");
    CHECK(b.has_value() && m.template key<std::optional>(*b) == "b" && m.template value<std::optional>(*b) == 20);
    CHECK(m.with(*b, [](int& v) { ++v; }) && m.template find<std::optional>("b") == 21);
    CHECK(m.erase("a") == 1 && m.template key<std::optional>(*b) == "c");     // what is there now
    CHECK(m.erase("b") == 1 && m.erase("d") == 1 && !m.template key<std::optional>(*b).has_value());
    CHECK(!m.with(*b, [](int&) { CHECK(false); }) && !m.template index_of<std::optional>("a").has_value());
    const auto last = m.erase(m.begin());       // on its own: the order of evaluation of `==` is not specified
    CHECK(last == m.end() && m.empty());

    std::mt19937               rng(20241124);
    std::map<std::string, int> model;
    for (int step = 0; step < 6000; ++step) {
        const std::string k = std::to_string(rng() % (2 * capacity));
        const int         v = static_cast<int>(rng() % 1000);
        switch (rng() % 4) {
        case 0: {
            const bool should = (model.size() < capacity) && !model.count(k);
            CHECK(m.insert(k, v).second == should);
            if (should) model[k] = v;
            break;
        }
        case 1:
        {
            const bool fits = model.count(k) || (model.size() < capacity);
            const auto at   = m.insert_or_assign(k, v).first;
            CHECK((at != m.end()) == fits);
            if (fits) {
                model[k] = v;
            }
        }
            break;
        case 2: CHECK(m.erase(k) == model.erase(k)); break;
        default: CHECK(m.with(k, [&](int& x) { x = v; }) == (model.count(k) > 0)); if (model.count(k)) model[k] = v;
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

struct descending {
    bool operator()(int x, int y) const { return x > y; }
};

void anytype()
{
    cobflatset<int, 8, descending> d;
    for (int x : { 2, 3, 1 }) {
        d.insert(x);
    }
    CHECK(std::vector<int>(d.begin(), d.end()) == (std::vector<int>{ 3, 2, 1 }) && *d.lower_bound(2) == 2);

    cobflatset<std::string, 4> strings;
    CHECK(strings.emplace(3u, 'b').second && strings.insert("a").second && strings.begin()->size() == 1);

    // what can only be moved
    cobflatmap<int, std::unique_ptr<int>, 4> up;
    CHECK(up.emplace(2, new int(20)).second && up.insert(1, std::make_unique<int>(10)).second);
    CHECK(up.insert(3, std::make_unique<int>(30)).second && up.erase(2) == 1);
    int sum = 0;
    up.for_each([&](int const& k, std::unique_ptr<int>& p) { sum += k * *p; });
    CHECK(sum == 10 + 90 && up.with(3, [](std::unique_ptr<int>& p) { *p = 1; }));

    // made when put in, destroyed when taken out, however they are moved about
    counted::alive = 0;
    {
        cobflatmap<int, counted, 8> m;
        CHECK(counted::alive == 0);
        for (int i = 0; i < 8; ++i) {
            CHECK(m.emplace(i * 5 % 8, i).second);
        }
        CHECK(counted::alive == 8 && !m.emplace(9, 9).second && counted::alive == 8);
        CHECK(!m.emplace(3, 33).second && counted::alive == 8);
        CHECK(m.erase(0) == 1 && m.erase(7) == 1 && m.erase(4) == 1 && counted::alive == 5);
        cobflatmap<int, counted, 8> c = m;
        CHECK(counted::alive == 10 && c.size() == 5);
        c.with(3, [](counted& x) { x.v = 99; });
        CHECK(m.find(3)->value.v != 99);
        m = c;
        CHECK(counted::alive == 10 && m.find(3)->value.v == 99);
        m.clear();
        CHECK(counted::alive == 5);
    }
    CHECK(counted::alive == 0);

    struct unset {};
    atbflatset<int, unset>      none;
    atbflatmap<int, int, unset> nomap;
    CHECK(!none.insert(1).second && none.insert(1).first == none.end() && none.empty() && none.full());
    CHECK(!nomap.insert(1, 1).second && nomap.insert_or_assign(1, 1).first == nomap.end() && nomap.erase(1) == 0);
}

int main()
{
    CHECK(atbdim<sized>::set(40));
    sets<cobflatset<int, 40>>(40);
    sets<cobflatset<int, 7>>(7);
    sets<atbflatset<int, sized>>(40);
    maps<cobflatmap<std::string, int, 40>>(40);
    maps<cobflatmap<std::string, int, 5>>(5);
    maps<atbflatmap<std::string, int, sized>>(40);
    anytype();
    return 0;
}
