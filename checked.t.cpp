// The containers with checked links: the link of an element that was erased
// is the link of nothing, also when a new element takes the slot it was in.

#include "atbfwdlist.hpp"
#include "atblist.hpp"
#include "atbmap.hpp"
#include "atbskiplist.hpp"
#include "atbtree.hpp"
#include "atbtreemap.hpp"
#include "cobfwdlist.hpp"
#include "coblist.hpp"
#include "cobmap.hpp"
#include "cobskiplist.hpp"
#include "cobtree.hpp"
#include "cobtreemap.hpp"

#include "cobtesthelp.hpp"

#include <algorithm>
#include <list>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>

struct sized {};    // the tag of the dimension, which is 32

using less    = cobhlp::less<int>;
using rng     = cobhlp::xorshift32;

// All of each one has to compile with checked links: inside, a link is not
// what it is outside, and taking one for the other is an error
template struct bndlist<int, cobbound<8>, true>;
template struct bndfwdlist<int, cobbound<8>, true>;
template struct bndskiplist<int, cobbound<8>, less, rng, true>;
template struct bndtree<int, cobbound<8>, less, true>;
template struct bndmapview<int, int, bndskiplist<cobhlp::keyval<int, int>, cobbound<8>, cobhlp::bykey<int, int, less>, rng, true>>;
template struct bndmapview<int, int, bndtree<cobhlp::keyval<int, int>, cobbound<8>, cobhlp::bykey<int, int, less>, true>>;
template struct bndlist<int, atbbound<sized>, true>;
template struct bndfwdlist<int, atbbound<sized>, true>;
template struct bndskiplist<int, atbbound<sized>, less, rng, true>;
template struct bndtree<int, atbbound<sized>, less, true>;

// A checked link is not a number, and not the link of a list that is not checked
static_assert(!std::is_constructible_v<cobcheckedlist<int, 8>::link, int>);
static_assert(!std::is_convertible_v<coblist<int, 8>::link, cobcheckedlist<int, 8>::link>);
static_assert(!std::is_convertible_v<cobcheckedlist<int, 8>::link, coblist<int, 8>::link>);
// Without the option, nothing changes
static_assert(std::is_same_v<coblist<int, 8>::link, cobint<0, 8>>);
static_assert(std::is_same_v<cobtree<int, 8>::link, cobint<0, 8>>);

template <class L> void lists()
{
    L l;
    CHECK(l.lbegin() == l.nil && l.lend() == l.nil && l.lback() == l.nil);
    const auto a = l.insert(l.lend(), 1);
    const auto b = l.insert(l.lend(), 2);
    CHECK(a != l.nil && a != b && l.live(a) && l.get(a) == 1 && l.contains(b));
    CHECK(l.fwd(a) == b && l.bck(b) == a && l.fwd(b) == l.nil && l.lbegin() == a && l.lfront() == a && l.lback() == b);

    CHECK(l.erase(a) == b && !l.live(a) && l.erase(a) == l.nil && l.get(a) == 0 && l.size() == 1);
    const auto c = l.insert(l.lbegin(), 3);     // takes the slot that `a` was in
    CHECK(l.live(c) && a != c && l.get(c) == 3);
    // the old link is of nothing, whatever it is used for
    CHECK(!l.live(a) && !l.contains(a) && l.get(a) == 0 && !l.set(a, 5) && l.get(c) == 3);
    CHECK(!l.with(a, [](int&) { CHECK(false); }) && l.fwd(a) == l.nil && l.bck(a) == l.nil);
    CHECK(l.insert(a, 9) == l.nil && l.erase(a) == l.nil && l.size() == 2);
    L other;
    CHECK(!l.splice(a, other));

    // so is an iterator that was at it
    auto it = l.begin();
    CHECK(*it == 3 && l.contains(it));
    l.template pop_front<std::optional>();
    l.push_front(7);
    CHECK(*it == 0 && !l.contains(it) && *l.begin() == 7 && ++it == l.end());

    // all that gives a link gives a good one
    CHECK(l.set(l.lbegin(), 8) && l.with(l.lback(), [](int& x) { x = 9; }));
    l.sort();
    l.reverse();
    CHECK(l.get(l.lbegin()) == 9 && l.get(l.lback()) == 8 && l.remove(8) == 1 && l.lbegin() == l.lback());
    l.clear();
    CHECK(!l.live(b) && !l.live(c) && l.empty());

    // and it is the same list otherwise
    std::mt19937   gen(1);
    std::list<int> model;
    const auto     room = l.max_size();
    for (int step = 0; step < 4000; ++step) {
        const int x = static_cast<int>(gen() % 10);
        switch (gen() % 6) {
        case 0: CHECK(l.push_back(x) == (model.size() < room)); if (model.size() < room) model.push_back(x); break;
        case 1: CHECK(l.push_front(x) == (model.size() < room)); if (model.size() < room) model.push_front(x); break;
        case 2:
            if (!model.empty()) {
                const auto k  = static_cast<std::ptrdiff_t>(gen() % model.size());
                const auto at = std::next(l.begin(), k);
                const auto mi = std::next(model.begin(), k);
                if (gen() % 2) {
                    const auto next = l.erase(at);
                    const auto mn   = model.erase(mi);
                    CHECK((mn == model.end()) ? (next == l.end()) : (*next == *mn));
                }
                else if (model.size() < room) {
                    CHECK(*l.insert(at, x) == x);
                    model.insert(mi, x);
                }
            }
            break;
        case 3: l.sort(); model.sort(); CHECK(l.unique() == model.unique()); break;
        case 4: l.reverse(); model.reverse(); break;
        default: CHECK(l.remove(x) == model.remove(x));
        }
        CHECK(std::equal(l.begin(), l.end(), model.begin(), model.end()) && l.size() == model.size());
    }
    L copy = l;
    CHECK(std::equal(copy.begin(), copy.end(), model.begin(), model.end()) && (l.empty() || copy.live(l.lbegin())));
}

template <class L> void fwdlists()
{
    L l;
    l.push_front(1);
    const auto head = l.lbegin();
    const auto n    = l.insert_after(head, 2);
    CHECK(l.live(n) && l.fwd(head) == n && l.erase_after(head) == l.nil && !l.live(n));
    const auto m = l.insert_after(head, 3);     // in the slot that `n` was in
    CHECK(l.live(m) && !l.live(n) && n != m && l.get(n) == 0 && l.get(m) == 3);
    CHECK(l.insert_after(n, 4) == l.nil && l.erase_after(n) == l.nil && l.fwd(n) == l.nil && l.size() == 2);
    auto it = l.begin();
    ++it;
    l.reverse();
    l.sort();
    CHECK(*it == 3 && l.contains(it) && l.remove(3) == 1 && *it == 0 && !l.contains(it));
    l.clear();
    CHECK(!l.live(head) && l.push_front(5) && !l.live(head) && l.live(l.lbegin()));
}

// A skiplist or a tree
template <class S> void sets()
{
    S          s;
    const auto a = s.insert(10).first;
    const auto b = s.insert(20).first;
    CHECK(a != s.end() && *a == 10 && s.contains(a) && s.size() == 2);
    const auto la = s.lbegin();
    CHECK(s.live(la) && s.contains(la) && s.fwd(la) != s.nil && s.fwd(s.fwd(la)) == s.nil);

    CHECK(s.erase(10) == 1 && !s.live(la) && !s.contains(a) && *a == 0);
    CHECK(s.insert(15).second);         // takes the slot that 10 was in
    CHECK(!s.live(la) && !s.contains(a) && *a == 0 && s.fwd(la) == s.nil && s.erase(la) == s.nil && s.size() == 2);
    CHECK(s.erase(a) == s.end() && s.size() == 2 && *b == 20 && s.contains(b));
    CHECK(*s.lower_bound(12) == 15 && *s.upper_bound(15) == 20 && *s.find(20) == 20 && s.count(15) == 1);
    CHECK(s.erase(s.lbegin()) != s.nil && s.erase(b) == s.end() && s.empty());

    std::mt19937  gen(2);
    std::set<int> model;
    const auto    room = s.max_size();
    for (int step = 0; step < 4000; ++step) {
        const int x = static_cast<int>(gen() % 60);
        if (gen() % 2) {
            const bool should = (model.size() < room) && !model.count(x);
            CHECK(s.insert(x).second == should);
            if (should) model.insert(x);
        }
        else if (gen() % 3) {
            CHECK(s.erase(x) == model.erase(x));
        }
        else if (model.count(x)) {
            const auto next = s.erase(s.find(x));
            const auto mn   = model.erase(model.find(x));
            CHECK((mn == model.end()) ? (next == s.end()) : (*next == *mn));
        }
        CHECK(s.size() == model.size() && std::equal(s.begin(), s.end(), model.begin(), model.end()));
    }
}

template <class M> void maps()
{
    M          m;
    const auto one = m.insert("one", 1).first;
    const auto two = m.emplace("two", 2).first;
    CHECK(m.live(one) && m.where("one") == one && m.lbegin() == one && m.fwd(one) == two);
    CHECK(m.template value<std::optional>(one) == 1 && m.template key<std::optional>(two) == "two");
    CHECK(m.with(one, [](int& v) { v = 11; }) && m.template find<std::optional>("one") == 11);
    CHECK(m.insert_or_assign("one", 12).first == one && m.lower_bound("o") == one && m.upper_bound("one") == two);

    CHECK(m.erase("one") == 1 && !m.live(one) && m.where("one") == m.nil);
    const auto ant = m.insert("ant", 5).first;      // takes the slot that "one" was in
    CHECK(m.live(ant) && ant != one && !m.live(one) && !m.contains(one));
    CHECK(!m.template value<std::optional>(one).has_value() && !m.template key<std::optional>(one).has_value());
    CHECK(!m.with(one, [](int&) { CHECK(false); }) && m.erase(one) == m.nil && m.fwd(one) == m.nil && m.size() == 2);
    CHECK(m.template value<std::optional>(ant) == 5 && m.erase(ant) == two && m.erase(two) == m.nil && m.empty());

    std::mt19937               gen(3);
    std::map<std::string, int> model;
    const auto                 room = m.max_size();
    for (int step = 0; step < 4000; ++step) {
        const std::string k = std::to_string(gen() % 50);
        const int         v = static_cast<int>(gen() % 100);
        switch (gen() % 3) {
        case 0:
            if (model.count(k) || (model.size() < room)) {
                CHECK(m.insert_or_assign(k, v).first != m.nil);
                model[k] = v;
            }
            break;
        case 1: CHECK(m.erase(k) == model.erase(k)); break;
        default: CHECK(m.with(k, [&](int& x) { x = v; }) == (model.count(k) > 0)); if (model.count(k)) model[k] = v;
        }
        CHECK(m.size() == model.size());
    }
    auto it = model.begin();
    m.for_each([&](std::string const& k, int const& v) {
        CHECK(it != model.end() && it->first == k && it->second == v);
        ++it;
    });
    CHECK(it == model.end());
}

int main()
{
    CHECK(atbdim<sized>::set(32));

    lists<cobcheckedlist<int, 16>>();
    lists<atbcheckedlist<int, sized>>();
    fwdlists<cobcheckedfwdlist<int, 16>>();
    fwdlists<atbcheckedfwdlist<int, sized>>();
    sets<cobcheckedskiplist<int, 32>>();
    sets<atbcheckedskiplist<int, sized>>();
    sets<cobcheckedtree<int, 32>>();
    sets<atbcheckedtree<int, sized>>();
    maps<cobcheckedmap<std::string, int, 32>>();
    maps<atbcheckedmap<std::string, int, sized>>();
    maps<cobcheckedtreemap<std::string, int, 32>>();
    maps<atbcheckedtreemap<std::string, int, sized>>();

    cobcheckedtree<int, 32> t;
    for (int i = 0; i < 32; ++i) {
        t.insert(i);
    }
    CHECK(t.sound() && t.height() == 6);
    return 0;
}
