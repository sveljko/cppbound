// What is built on the pool can have the pool keep which slots hold an
// element as a bit each (the "hive" names). That must change nothing but the
// room taken: each one is run against a model, or against its twin that is
// not a hive, and has to do the same.

#include "atbfwdlist.hpp"
#include "atblist.hpp"
#include "atblru.hpp"
#include "atbmap.hpp"
#include "atbpool.hpp"
#include "atbskiplist.hpp"
#include "atbtmrlist.hpp"
#include "atbtmrmill.hpp"
#include "atbtmrwheel.hpp"
#include "atbtree.hpp"
#include "atbtreemap.hpp"
#include "cobfwdlist.hpp"
#include "coblist.hpp"
#include "coblru.hpp"
#include "cobmap.hpp"
#include "cobpool.hpp"
#include "cobskiplist.hpp"
#include "cobtmrlist.hpp"
#include "cobtmrmill.hpp"
#include "cobtmrwheel.hpp"
#include "cobtree.hpp"
#include "cobtreemap.hpp"

#include "cobtesthelp.hpp"

#include <algorithm>
#include <chrono>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

using ms = std::chrono::milliseconds;

struct sized {};                        // the tag of the dimension, which is 200
template <int N> struct timers {};      // the tag of the dimension N

using less = cobhlp::less<int>;
using rng  = cobhlp::xorshift32;

// All of each one has to compile as a hive, checked or not, with either bound
#define HIVES(B)                                                                                                     \
    template struct bndlist<int, B, false, true>;                                                                    \
    template struct bndlist<int, B, true, true>;                                                                     \
    template struct bndfwdlist<int, B, false, true>;                                                                 \
    template struct bndfwdlist<int, B, true, true>;                                                                  \
    template struct bndskiplist<int, B, less, rng, false, true>;                                                     \
    template struct bndskiplist<int, B, less, rng, true, true>;                                                      \
    template struct bndtree<int, B, less, false, true>;                                                              \
    template struct bndtree<int, B, less, true, true>;                                                               \
    template struct bndlru<int, int, B, std::hash<int>, std::equal_to<int>, true>;                                   \
    template class bndtmrlist<int, B, ms, false, true>;                                                              \
    template class bndtmrlist<int, B, ms, true, true>;                                                               \
    template class bndtmrwheel<int, B, ms, 4, false, true>;                                                          \
    template class bndtmrmill<int, B, ms, 2, 4, true, true>;
HIVES(cobbound<8>)
HIVES(atbbound<sized>)
template class bndtmrexactmill<int, cobbound<8>, cobbound<16>, ms, 2, 4, true>;

// The names are those of the options
static_assert(std::is_same_v<cobhivelist<int, 8>, bndlist<int, cobbound<8>, false, true>>);
static_assert(std::is_same_v<cobcheckedhivelist<int, 8>, bndlist<int, cobbound<8>, true, true>>);
static_assert(std::is_same_v<atbcheckedhivetree<int, sized>, bndtree<int, atbbound<sized>, less, true, true>>);
static_assert(std::is_same_v<cobhivetmrmill<int, 512, ms, 3, 64, true>, cobcheckedhivetmrmill<int, 512, ms, 3, 64, true>>);
static_assert(!std::is_same_v<cobhivetmrmill<int, 512, ms, 3, 64, true>, cobtmrmill<int, 512, ms, 3, 64, true>>);
// A link is what it is without the option...
static_assert(std::is_same_v<cobhivelist<int, 8>::link, coblist<int, 8>::link>);
static_assert(std::is_same_v<cobhivetree<int, 8>::link, cobint<0, 8>>);
static_assert(!std::is_constructible_v<cobcheckedhivelist<int, 8>::link, int>);
// ...and the room is less: a bit, not a byte, for each slot
static_assert(sizeof(cobhivelist<char, 1024>) + 800 <= sizeof(coblist<char, 1024>));
static_assert(sizeof(cobhivefwdlist<char, 1024>) + 800 <= sizeof(cobfwdlist<char, 1024>));
static_assert(sizeof(cobhivetree<char, 1024>) + 800 <= sizeof(cobtree<char, 1024>));
static_assert(sizeof(cobhiveskiplist<char, 1024>) + 800 <= sizeof(cobskiplist<char, 1024>));
static_assert(sizeof(cobcheckedhivelist<char, 1024>) + 800 <= sizeof(cobcheckedlist<char, 1024>));
static_assert(sizeof(cobhivelru<int, int, 1024>) + 800 <= sizeof(coblru<int, int, 1024>));
static_assert(sizeof(cobhivetmrlist<int, 1024, ms>) + 800 <= sizeof(cobtmrlist<int, 1024, ms>));
static_assert(sizeof(cobhivetmrmill<int, 1024, ms, 2, 8, true>) + 800 <= sizeof(cobtmrmill<int, 1024, ms, 2, 8, true>));
// Going from object to object by handles is of the pool only
template <class C>
concept walkable = requires(C const& c) { c.first(); };
static_assert(walkable<cobhive<int, 8>> && !walkable<cobhivelist<int, 8>> && !walkable<cobhivetree<int, 8>>);


// `checked`: with checked links, the link of what is gone is the link of nothing
template <class L> void lists(bool checked)
{
    L l;
    CHECK(l.empty() && l.lbegin() == l.nil && l.lback() == l.nil && l.size() == 0);
    const auto a = l.insert(l.lend(), 1);
    const auto b = l.insert(l.lend(), 2);
    CHECK(l.live(a) && l.live(b) && l.fwd(a) == b && l.bck(b) == a && l.get(a) == 1 && l.lback() == b);
    CHECK(l.erase(a) == b && !l.live(a) && l.size() == 1);
    const auto c = l.insert(l.lbegin(), 3);     // takes the slot that `a` was in
    CHECK(l.live(c) && l.get(c) == 3 && l.live(a) != checked);
    l.clear();
    CHECK(l.empty() && !l.live(b) && !l.live(c));

    std::mt19937   gen(11);
    std::list<int> model;
    const auto     room = l.max_size();
    for (int step = 0; step < 6000; ++step) {
        const int x = static_cast<int>(gen() % 10);
        switch (gen() % 7) {
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
                    const auto put = l.insert(at, x);
                    CHECK(*put == x);
                    model.insert(mi, x);
                }
            }
            break;
        case 3: l.sort(); model.sort(); CHECK(l.unique() == model.unique()); break;
        case 4: l.reverse(); model.reverse(); break;
        case 5:
            if (!model.empty()) {
                const auto back = l.template pop_back<std::optional>();
                CHECK(back == model.back());
                model.pop_back();
            }
            break;
        default: CHECK(l.remove(x) == model.remove(x));
        }
        CHECK(std::equal(l.begin(), l.end(), model.begin(), model.end()) && l.size() == model.size());
        CHECK(l.full() == (model.size() == room));
    }
    // a copy has the same elements at the same links, and is apart
    L copy = l;
    CHECK(std::equal(copy.begin(), copy.end(), model.begin(), model.end()) && (l.empty() || copy.live(l.lbegin())));
    copy.clear();
    CHECK(copy.empty() && l.size() == model.size());
    copy = l;
    CHECK(std::equal(copy.begin(), copy.end(), model.begin(), model.end()));

    // all of one list into another
    L other;
    other.push_back(100);
    const auto total = l.size() + 1;
    CHECK(other.splice(other.lend(), l) == (total <= room));
    if (total <= room) {
        CHECK(l.empty() && other.size() == total && *other.begin() == 100);
    }
}

template <class L> void fwdlists(bool checked)
{
    L l;
    l.push_front(1);
    const auto head = l.lbegin();
    const auto n    = l.insert_after(head, 2);
    CHECK(l.live(n) && l.fwd(head) == n && l.erase_after(head) == l.nil && !l.live(n));
    const auto m = l.insert_after(head, 3);     // in the slot that `n` was in
    CHECK(l.live(m) && l.get(m) == 3 && l.live(n) != checked);
    l.clear();

    std::mt19937           gen(12);
    std::forward_list<int> model;
    std::size_t            count = 0;
    const auto             room  = l.max_size();
    for (int step = 0; step < 6000; ++step) {
        const int x = static_cast<int>(gen() % 10);
        switch (gen() % 6) {
        case 0:
            CHECK(l.push_front(x) == (count < room));
            if (count < room) { model.push_front(x); ++count; }
            break;
        case 1:
            if (count > 0) {
                const auto k  = static_cast<std::ptrdiff_t>(gen() % count);
                const auto at = std::next(l.begin(), k);
                const auto mi = std::next(model.begin(), k);
                if (gen() % 2) {
                    const bool had = std::next(mi) != model.end();
                    l.erase_after(at);
                    if (had) { model.erase_after(mi); --count; }
                }
                else if (count < room) {
                    const auto put = l.insert_after(at, x);
                    CHECK(*put == x);
                    model.insert_after(mi, x);
                    ++count;
                }
            }
            break;
        case 2: l.sort(); model.sort(); break;
        case 3: l.reverse(); model.reverse(); break;
        case 4:
            if (count > 0) {
                const auto front = l.template pop_front<std::optional>();
                CHECK(front == model.front());
                model.pop_front();
                --count;
            }
            break;
        default: {
            const auto gone = l.remove(x);
            count -= gone;
            model.remove(x);
        }
        }
        CHECK(std::equal(l.begin(), l.end(), model.begin(), model.end()) && l.size() == count);
    }
    L copy = l;
    CHECK(std::equal(copy.begin(), copy.end(), model.begin(), model.end()));
}

// A skiplist or a tree
template <class S> void sets(bool checked)
{
    S          s;
    const auto a  = s.insert(10).first;
    const auto la = s.lbegin();
    CHECK(*a == 10 && s.live(la) && s.erase(10) == 1 && !s.live(la));
    CHECK(s.insert(15).second && s.live(la) != checked);    // takes the slot that 10 was in
    s.clear();
    CHECK(s.empty() && !s.live(la));

    std::mt19937  gen(13);
    std::set<int> model;
    const auto    room = s.max_size();
    for (int step = 0; step < 8000; ++step) {
        const int x = static_cast<int>(gen() % (room + room / 2));
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
        CHECK(s.size() == model.size() && s.count(x) == model.count(x));
        if (step % 16 == 0) {
            CHECK(std::equal(s.begin(), s.end(), model.begin(), model.end()));
            const auto lb = model.lower_bound(x);
            const auto ub = model.upper_bound(x);
            CHECK((lb == model.end()) ? (s.lower_bound(x) == s.end()) : (*s.lower_bound(x) == *lb));
            CHECK((ub == model.end()) ? (s.upper_bound(x) == s.end()) : (*s.upper_bound(x) == *ub));
        }
    }
    S copy = s;
    CHECK(std::equal(copy.begin(), copy.end(), model.begin(), model.end()));
    // by links, it is the same walk
    auto mi = model.begin();
    for (auto l = s.lbegin(); l != s.nil; l = s.fwd(l), ++mi) {
        CHECK(s.live(l) && mi != model.end());
    }
    CHECK(mi == model.end());
}

template <class M> void maps()
{
    M                          m;
    std::mt19937               gen(14);
    std::map<std::string, int> model;
    const auto                 room = m.max_size();
    for (int step = 0; step < 6000; ++step) {
        const std::string k = std::to_string(gen() % (room + room / 2));
        const int         v = static_cast<int>(gen() % 100);
        switch (gen() % 4) {
        case 0:
            if (model.count(k) || (model.size() < room)) {
                const auto put = m.insert_or_assign(k, v);
                CHECK(put.first != m.nil && put.second == !model.count(k));
                model[k] = v;
            }
            else {
                CHECK(m.insert(k, v).first == m.nil);
            }
            break;
        case 1: CHECK(m.erase(k) == model.erase(k)); break;
        case 2: CHECK(m.with(k, [&](int& x) { x = v; }) == (model.count(k) > 0)); if (model.count(k)) model[k] = v; break;
        default: {
            const auto found = m.template find<std::optional>(k);
            CHECK(found.has_value() == (model.count(k) > 0) && (!found || *found == model[k]));
        }
        }
        CHECK(m.size() == model.size() && m.contains(k) == (model.count(k) > 0));
    }
    auto it = model.begin();
    m.for_each([&](std::string const& k, int const& v) {
        CHECK(it != model.end() && it->first == k && it->second == v);
        ++it;
    });
    CHECK(it == model.end());
    const M copy = m;
    CHECK(copy.size() == model.size());
    for (auto const& e : copy) {
        CHECK(model.at(e.key) == e.value);
    }
}

// The cache, and the one that is not a hive: whatever is done to both, they are the same
template <class H, class P> void caches()
{
    H            h;
    P            p;
    std::mt19937 gen(15);
    const auto   keys = static_cast<unsigned>(h.max_size() * 2);
    for (int step = 0; step < 8000; ++step) {
        const int k = static_cast<int>(gen() % keys);
        const int v = static_cast<int>(gen() % 1000);
        switch (gen() % 6) {
        case 0:
        case 1: {
            std::vector<int> hgone, pgone;
            CHECK(h.put(k, v, [&](int const& key, int&) { hgone.push_back(key); }));
            CHECK(p.put(k, v, [&](int const& key, int&) { pgone.push_back(key); }));
            CHECK(hgone == pgone);
            break;
        }
        case 2: CHECK(h.template get<std::optional>(k) == p.template get<std::optional>(k)); break;
        case 3: CHECK(h.template peek<std::optional>(k) == p.template peek<std::optional>(k)); break;
        case 4: CHECK(h.erase(k) == p.erase(k)); break;
        default: CHECK(h.with(k, [&](int& x) { x = v; }) == p.with(k, [&](int& x) { x = v; }));
        }
        CHECK(h.size() == p.size() && h.contains(k) == p.contains(k));
        CHECK(h.template eldest<std::optional>() == p.template eldest<std::optional>());
    }
    std::vector<std::pair<int, int>> hall, pall;
    h.for_each([&](int const& k, int const& v) { hall.emplace_back(k, v); });
    p.for_each([&](int const& k, int const& v) { pall.emplace_back(k, v); });
    CHECK(hall == pall && !hall.empty());
    const H copy = h;
    std::vector<std::pair<int, int>> call;
    copy.for_each([&](int const& k, int const& v) { call.emplace_back(k, v); });
    CHECK(call == hall);
    h.clear();
    CHECK(h.empty() && !h.template eldest<std::optional>().has_value());
}

// The timers, and the ones that are not a hive: started, stopped and let to
// expire the same way, they report the same
template <class H, class P> void timing(unsigned longest, unsigned ids)
{
    H            h;
    P            p;
    std::mt19937 gen(16);
    using hindex = decltype(h.start(0, ms{ 1 }));
    using pindex = decltype(p.start(0, ms{ 1 }));
    std::vector<std::pair<hindex, pindex>> started;
    std::size_t                            reported = 0;
    for (int step = 0; step < 6000; ++step) {
        const int id = static_cast<int>(gen() % ids);
        switch (gen() % 12) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5: {
            const ms d{ 1 + gen() % longest };
            started.emplace_back(h.start(id, d), p.start(id, d));
            break;
        }
        case 6: CHECK(h.stop(id) == p.stop(id)); break;
        case 7:
            if (!started.empty()) {
                const auto at = gen() % started.size();
                h.stop(started[at].first);
                p.stop(started[at].second);
                started.erase(started.begin() + static_cast<std::ptrdiff_t>(at));
            }
            break;
        case 8: CHECK(h.template stop_first<std::optional>() == p.template stop_first<std::optional>()); break;
        default: {
            // those that expire together are reported in an order that goes by the slots
            const ms         elapsed{ 1 + gen() % 3 };
            std::vector<int> hx, px;
            h.process_expired(elapsed, [&](int x) { hx.push_back(x); });
            p.process_expired(elapsed, [&](int x) { px.push_back(x); });
            CHECK(hx == px);
            reported += hx.size();
            started.clear();    // an index of what has expired is not to be used, unless checked
        }
        }
    }
    CHECK(reported > 100);
}

// The same, with indices kept over expiry: for those that are checked
template <class H, class P> void timingchecked(unsigned longest)
{
    H            h;
    P            p;
    std::mt19937 gen(17);
    using hindex = decltype(h.start(0, ms{ 1 }));
    using pindex = decltype(p.start(0, ms{ 1 }));
    std::vector<std::pair<hindex, pindex>> started;
    int                                    id = 0;
    for (int step = 0; step < 6000; ++step) {
        switch (gen() % 4) {
        case 0:
        case 1: {
            const ms d{ 1 + gen() % longest };
            started.emplace_back(h.start(id, d), p.start(id, d));
            ++id;
            break;
        }
        case 2:
            if (!started.empty()) {     // running or not: an old index stops nothing else
                const auto at = gen() % started.size();
                h.stop(started[at].first);
                p.stop(started[at].second);
            }
            break;
        default: {
            std::vector<int> hx, px;
            h.process_expired(ms{ 1 }, [&](int x) { hx.push_back(x); });
            p.process_expired(ms{ 1 }, [&](int x) { px.push_back(x); });
            CHECK(hx == px);
        }
        }
    }
}

// The bits are in words of 64: what is around the edge of a word
template <class P> void edges()
{
    P                                       p;
    std::vector<typename P::handle>         all;
    const auto                              room = p.max_size();
    for (unsigned i = 0; i < room; ++i) {
        all.push_back(p.make(static_cast<int>(i)));
        CHECK(p.live(all.back()));
    }
    CHECK(p.full() && p.make(0) == p.nil);
    // leave only those next to an edge, and the last one
    std::vector<int> left;
    for (unsigned i = 0; i < room; ++i) {
        const bool keep = (i % 64 == 0) || (i % 64 == 63) || (i + 1 == room);
        if (keep) {
            left.push_back(static_cast<int>(i));
        }
        else {
            CHECK(p.drop(all[i]) && !p.live(all[i]));
        }
    }
    std::vector<int> seen;
    for (auto h = p.first(); h != p.nil; h = p.next(h)) {
        seen.push_back(p.get(h));
    }
    std::vector<int> each;
    p.for_each([&](int const& x) { each.push_back(x); });
    std::sort(seen.begin(), seen.end());
    std::sort(each.begin(), each.end());
    CHECK(seen == left && each == left && p.size() == left.size());
    const P copy = p;
    std::vector<int> copied;
    for (auto h = copy.first(); h != copy.nil; h = copy.next(h)) {
        copied.push_back(copy.get(h));
    }
    std::sort(copied.begin(), copied.end());
    CHECK(copied == left);
    p.clear();
    CHECK(p.empty() && p.first() == p.nil && p.next(all[0]) == p.nil);
    for (unsigned i = 0; i < room; ++i) {
        CHECK(p.make(1) != p.nil);
    }
    CHECK(p.full());
}

int main()
{
    CHECK(atbdim<sized>::set(200));
    for (bool set : { atbdim<timers<8>>::set(8), atbdim<timers<64>>::set(64), atbdim<timers<512>>::set(512) }) {
        CHECK(set);
    }

    lists<cobhivelist<int, 16>>(false);
    lists<cobhivelist<int, 130>>(false);
    lists<cobcheckedhivelist<int, 16>>(true);
    lists<atbhivelist<int, sized>>(false);
    lists<atbcheckedhivelist<int, sized>>(true);
    fwdlists<cobhivefwdlist<int, 16>>(false);
    fwdlists<cobcheckedhivefwdlist<int, 70>>(true);
    fwdlists<atbhivefwdlist<int, sized>>(false);
    fwdlists<atbcheckedhivefwdlist<int, sized>>(true);
    sets<cobhiveskiplist<int, 32>>(false);
    sets<cobcheckedhiveskiplist<int, 130>>(true);
    sets<atbhiveskiplist<int, sized>>(false);
    sets<atbcheckedhiveskiplist<int, sized>>(true);
    sets<cobhivetree<int, 32>>(false);
    sets<cobcheckedhivetree<int, 130>>(true);
    sets<atbhivetree<int, sized>>(false);
    sets<atbcheckedhivetree<int, sized>>(true);
    maps<cobhivemap<std::string, int, 32>>();
    maps<cobcheckedhivemap<std::string, int, 70>>();
    maps<atbhivemap<std::string, int, sized>>();
    maps<atbcheckedhivemap<std::string, int, sized>>();
    maps<cobhivetreemap<std::string, int, 32>>();
    maps<cobcheckedhivetreemap<std::string, int, 70>>();
    maps<atbhivetreemap<std::string, int, sized>>();
    maps<atbcheckedhivetreemap<std::string, int, sized>>();
    caches<cobhivelru<int, int, 24>, coblru<int, int, 24>>();
    caches<cobhivelru<int, int, 130>, coblru<int, int, 130>>();
    caches<atbhivelru<int, int, sized>, atblru<int, int, sized>>();

    {
        cobhivetree<int, 130> t;
        for (int i = 0; i < 130; ++i) {
            t.insert(i);
        }
        CHECK(t.sound() && t.height() == 8 && t.full());
        // elements of any type
        cobhivelist<std::unique_ptr<int>, 4> up;
        CHECK(up.emplace_back(new int(2)) && up.with(up.lfront(), [](std::unique_ptr<int>& x) { *x = 3; }));
        cobhivemap<std::string, counted, 4> counts;
        CHECK(counts.emplace("a", 1).second && counts.emplace("b", 2).second && counted::alive == 2);
        counts.erase("a");
        CHECK(counted::alive == 1);
    }
    CHECK(counted::alive == 0);

    timing<cobhivetmrlist<int, 64, ms>, cobtmrlist<int, 64, ms>>(40, 24);
    timing<cobhivetmrwheel<int, 64, ms, 4>, cobtmrwheel<int, 64, ms, 4>>(40, 24);
    timing<cobhivetmrmill<int, 512, ms, 3, 8>, cobtmrmill<int, 512, ms, 3, 8>>(500, 24);
    timing<cobhivetmrmill<int, 64, ms, 3, 8, true>, cobtmrmill<int, 64, ms, 3, 8, true>>(500, 24);
    timing<cobcheckedhivetmrlist<int, 64, ms>, cobcheckedtmrlist<int, 64, ms>>(40, 24);
    timing<cobcheckedhivetmrwheel<int, 64, ms, 4>, cobcheckedtmrwheel<int, 64, ms, 4>>(40, 24);
    timing<cobcheckedhivetmrmill<int, 512, ms, 3, 8>, cobcheckedtmrmill<int, 512, ms, 3, 8>>(500, 24);
    timing<atbhivetmrlist<int, timers<64>, ms>, atbtmrlist<int, timers<64>, ms>>(40, 24);
    timing<atbhivetmrwheel<int, timers<64>, ms, 4>, atbtmrwheel<int, timers<64>, ms, 4>>(40, 24);
    timing<atbhivetmrmill<int, timers<512>, ms, 3, 8>, atbtmrmill<int, timers<512>, ms, 3, 8>>(500, 24);
    timing<atbhivetmrmill<int, timers<64>, ms, 3, 8, true>, atbtmrmill<int, timers<64>, ms, 3, 8, true>>(500, 24);
    timing<atbcheckedhivetmrlist<int, timers<64>, ms>, atbcheckedtmrlist<int, timers<64>, ms>>(40, 24);
    timing<atbcheckedhivetmrwheel<int, timers<64>, ms, 4>, atbcheckedtmrwheel<int, timers<64>, ms, 4>>(40, 24);
    timing<atbcheckedhivetmrmill<int, timers<512>, ms, 3, 8>, atbcheckedtmrmill<int, timers<512>, ms, 3, 8>>(500, 24);

    timingchecked<cobcheckedhivetmrlist<int, 64, ms>, cobcheckedtmrlist<int, 64, ms>>(40);
    timingchecked<cobcheckedhivetmrwheel<int, 64, ms, 4>, cobcheckedtmrwheel<int, 64, ms, 4>>(40);
    timingchecked<cobcheckedhivetmrmill<int, 512, ms, 3, 8>, cobcheckedtmrmill<int, 512, ms, 3, 8>>(500);
    timingchecked<cobhivetmrmill<int, 64, ms, 3, 8, true>, cobtmrmill<int, 64, ms, 3, 8, true>>(500);
    timingchecked<atbhivetmrmill<int, timers<64>, ms, 3, 8, true>, atbtmrmill<int, timers<64>, ms, 3, 8, true>>(500);

    edges<cobhive<int, 64>>();
    edges<cobhive<int, 65>>();
    edges<cobhive<int, 127>>();
    edges<cobhive<int, 129>>();
    edges<cobcheckedhive<int, 192>>();
    edges<atbhive<int, sized>>();
    edges<atbcheckedhive<int, sized>>();
    return 0;
}
