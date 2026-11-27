// The corners that the tests of each class do not get to: members that are
// seldom used, the read-only overloads, and what happens at the edges.

#include "atbarray.hpp"
#include "atbbitset.hpp"
#include "atbmatrix.hpp"
#include "atbstring.hpp"
#include "cobarray.hpp"
#include "cobbitset.hpp"
#include "cobflatmap.hpp"
#include "cobflatset.hpp"
#include "cobfwdlist.hpp"
#include "cobhash.hpp"
#include "coblist.hpp"
#include "cobmap.hpp"
#include "cobmatrix.hpp"
#include "cobpool.hpp"
#include "cobskiplist.hpp"
#include "cobspsc.hpp"
#include "cobstring.hpp"
#include "cobtmrleanmill.hpp"
#include "cobtmrleanring.hpp"
#include "cobtmrlist.hpp"
#include "cobtmrmill.hpp"
#include "cobtmrwheel.hpp"
#include "cobtree.hpp"
#include "cobtreemap.hpp"
#include "cobvector.hpp"
#include "contingent.hpp"
#include "optptr.hpp"
#include "optshared.hpp"
#include "optunique.hpp"
#include "perhaps.hpp"

#include "cobtesthelp.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using ms = std::chrono::milliseconds;

struct four {};     // the tag of the dimension 4
struct wide {};     // the tag of the dimension 100
struct pt {
    int x = 0;
    int y = 0;
};

void arrays()
{
    // made at run time, not in a constant expression
    int        seed = 3;
    const auto same = cobarray<int, 4>::filled(seed);
    const auto each = cobarray<int, 4>::generate([&](cobint<0, 3> i) { return seed * i.get(); });
    CHECK(same.get(cobic<3>) == 3 && each.get(cobic<2>) == 6 && each.front() == 0 && each.back() == 9);
    const auto asame = atbarray<int, four>::filled(seed);
    const auto aeach = atbarray<int, four>::generate([&](atbidx<four> i) { return static_cast<int>(i.get()) + seed; });
    CHECK(asame.size() == 4 && asame.back() == 3 && aeach.front() == 3 && aeach.back() == 6);
    CHECK(std::accumulate(aeach.begin(), aeach.end(), 0) == 18);

    // members, through an iterator
    cobarray<pt, 3> pts;
    pts.set(cobic<1>, &pt::x, 7);
    auto it = pts.begin();
    ++it;
    CHECK(it->x == 7 && pts.cbegin()->x == 0);
    it->y = 8;
    CHECK(pts.get(cobic<1>, &pt::y) == 8 && pts.grab(cobic<1>)->y == 8);
    CHECK(!pts.maybe_set(3, pt{}) && pts.maybe_set(2, pt{ 1, 2 }) && pts.get(cobic<2>).y == 2);

    // going over the indices of a tag by hand
    auto       walk = atbidx<four>::range();
    auto       at   = walk.begin();
    const auto was  = at++;
    CHECK((*was).get() == 0 && (*at).get() == 1);
    auto last = atbidx<four>::last<std::optional>();
    CHECK(last.has_value() && last->get() == 3 && !last->advance() && last->ebb() && last->get() == 2);
    CHECK(!atbidx<four>::be<std::optional>(4).has_value() && atbidx<four>::first<std::optional>()->get() == 0);
}

void matrices()
{
    int        seed = 2;
    const auto each = cobmatrix<int, 2, 3>::generate(
        [&](cobmatrix<int, 2, 3>::index i) { return seed * (10 * i.irow.get() + i.icolumn.get()); });
    CHECK(each.get({ cobic<1>, cobic<2> }) == 24 && each.front() == 0 && each.back() == 24);
    const auto row = each.get_row(cobic<1>);
    CHECK(row.get(cobic<0>) == 20 && row.get(cobic<2>) == 24);
    cobmatrix<int, 2, 3> m = cobmatrix<int, 2, 3>::filled(seed);
    const int            nine = 9;
    CHECK(m.set({ cobic<0>, cobic<1> }, nine) == 9 && m.get({ cobic<0>, cobic<1> }) == 9);
    m.set_row(cobic<1>, row);
    CHECK(m.get({ cobic<1>, cobic<1> }) == 22 && !(m == each) && m.rows() == 2 && m.columns() == 3);
    CHECK(!m.maybe_set(2, 0, 1) && !m.maybe_set(0, 3, 1) && m.maybe_set(1, 2, 5) && m.back() == 5);

    cobmatrix<pt, 2, 2> pts;
    pts.set({ cobic<1>, cobic<1> }, &pt::x, 4);
    auto it = pts.begin();
    it += 3;
    CHECK(it->x == 4 && pts.cbegin()->x == 0 && pts.get({ cobic<1>, cobic<1> }, &pt::x) == 4);

    atbmatrix<int, four, four> am;
    am.fill(1);
    const auto r1 = *atbidx<four>::be<std::optional>(1);
    const auto c3 = *atbidx<four>::be<std::optional>(3);
    am.set({ r1, c3 }, 7);
    const auto arow = am.get_row(r1);
    CHECK(arow.get(c3) == 7 && arow.front() == 1 && std::accumulate(am.begin(), am.end(), 0) == 22);
    const auto agen = atbmatrix<int, four, four>::generate(
        [](atbmatrix<int, four, four>::index i) { return static_cast<int>(i.irow.get() * 4 + i.icolumn.get()); });
    CHECK(agen.get({ r1, c3 }) == 7 && agen.back() == 15 && !(agen == am));
}

void bitsets()
{
    // at run time: the size of this one is not known before
    atbbitset<wide> a;
    atbbitset<wide> b;
    const auto      at = [](std::size_t i) { return *atbidx<wide>::be<std::optional>(i); };
    CHECK(a.none() && !a.any() && !a.all() && a.size() == 100 && a == b);
    a.set(at(3));
    a.set(at(64));
    a.set(at(99));
    b.set(at(64));
    CHECK(!(a == b) && a.count() == 3 && a.test(at(64)) && !a.test(at(63)));
    CHECK((a & b) == b && (a | b) == a && (a ^ b).count() == 2 && !(a ^ b).test(at(64)));
    CHECK((~a).count() == 97 && !(~a).test(at(99)) && (~a | a).all());
    b ^= a;
    b |= a;
    b &= a;
    CHECK(b == a);
    std::vector<std::size_t> seen;
    for (auto i = a.first<std::optional>(); i; i = a.next<std::optional>(*i)) {
        seen.push_back(i->get());
    }
    CHECK((seen == std::vector<std::size_t>{ 3, 64, 99 }));
    seen.clear();
    a.for_each([&](atbidx<wide> i) { seen.push_back(i.get()); });
    CHECK(seen.size() == 3 && !a.next<std::optional>(at(99)).has_value());
    CHECK(a.maybe_set(5) && !a.maybe_set(100) && !a.maybe_set(-1) && a.maybe_set(3, false) && a.count() == 3);
    a.flip(at(5));
    a.reset(at(64));
    a.flip();
    CHECK(a.count() == 99 && !a.test(at(99)));
    a.set();
    CHECK(a.all());
    a.reset();
    CHECK(a.none() && !a.first<std::optional>().has_value());

    cobbitset<70> c;
    cobbitset<70> d;
    c.set(cobic<69>);
    CHECK(!(c == d) && (c ^ c) == d && (~d).all() && c.maybe_set(0) && c.count() == 2);
}

void strings()
{
    atbstring<wide> s;
    CHECK(s.assign("hello, world") && s.capacity() == 100);
    const auto part = s.substr(7, 3);
    CHECK(part == "wor" && s.substr(7) == "world" && s.substr(12).empty() && s.substr(13).empty());
    CHECK(s.substr() == s && s.substr(0, 0).empty());
    cobstring<8> c = "abc";
    const auto   tail = c.substr(1);
    CHECK(tail == "bc" && tail.capacity() == 8 && c.substr(4).empty());
    std::string out;
    CHECK(c.insert(1, 2, '-') && c == "a--bc" && !c.insert(6, 1, 'x') && !c.insert(0, 4, 'x') && c == "a--bc");
    CHECK(c.assign(3, 'z') && c == "zzz" && !c.assign(9, 'z') && c == "zzz" && c.resize(5, 'y') && c == "zzzyy");
    CHECK(c.resize(1) && c == "z" && !c.resize(9) && c.pop_back<std::optional>() == 'z' && !c.pop_back<std::optional>());
    CHECK(!c.front<std::optional>() && !c.back<std::optional>() && !c.set(cobic<0>, 'a') && c.get(cobic<7>) == '\0');
}

template <class L> void const_lists()
{
    L l;
    l.push_front(pt{ 1, 2 });
    l.push_front(pt{ 3, 4 });
    const L& cl  = l;
    int      sum = 0;
    cl.for_each([&](pt const& p) { sum += p.x; });
    CHECK(sum == 4 && cl.begin()->x == 3 && l.begin()->y == 4 && cl.cbegin()->y == 4);
    l.begin()->y = 5;
    CHECK(cl.get(cl.lbegin()).y == 5 && cl.template front<std::optional>()->x == 3);
    CHECK(cl.with(cl.lbegin(), [&](pt const& p) { sum += p.y; }) && sum == 9);
    CHECK(!cl.with(cl.lend(), [](pt const&) { CHECK(false); }) && cl.contains(cl.begin()) && !cl.contains(cl.end()));
}

void lists()
{
    const_lists<coblist<pt, 4>>();
    const_lists<cobfwdlist<pt, 4>>();

    cobfwdlist<int, 4> f;
    f.push_front(1);
    const int two = 2;
    auto      it  = f.insert_after(f.cbegin(), two);    // a copy, by an iterator
    CHECK(*it == 2 && f.size() == 2);
    it = f.insert_after(f.cend(), two);                 // after no element: nowhere
    CHECK(it == f.end() && f.size() == 2);
    CHECK(f.insert_after(f.lend(), 3) == f.nil && f.erase_after(f.lend()) == f.nil && f.size() == 2);
    cobfwdlist<int, 4> other;
    CHECK(other.insert_after(f.cbegin(), two) == other.end() || other.size() <= 1);     // of another list
    CHECK(f.erase_after(f.cbegin()) == f.end() && f.size() == 1);
    CHECK(f.pop_front<std::optional>() == 1 && !f.pop_front<std::optional>() && f.empty());

    // a pool that can only be read
    cobpool<int, 4> p;
    const auto      a = p.make(1);
    const auto      b = p.make(2);
    p.drop(a);
    const cobpool<int, 4>& cp  = p;
    int                    sum = 0;
    cp.for_each([&](int const& x) { sum += x; });
    CHECK(sum == 2 && cp.with(b, [&](int const& x) { sum += x; }) && sum == 4 && !cp.with(a, [](int const&) {}));
    const cobcheckedpool<int, 4> ccp;
    ccp.for_each([](int const&) { CHECK(false); });
    CHECK(ccp.empty() && !ccp.with(ccp.nil, [](int const&) {}) && ccp.get(ccp.nil) == 0);
}

// A skiplist or a tree
template <class S> void sets()
{
    S s;
    for (int x : { 5, 1, 3 }) {
        s.insert(x);
    }
    typename S::iterator none;      // of no set: at the end of one that is empty
    CHECK(*none == 0 && !s.contains(none) && s.erase(none) == s.end() && s.size() == 3);
    auto       it  = s.cbegin();
    const auto was = it++;
    CHECK(*was == 1 && *it == 3 && s.cend() == s.end() && std::distance(s.cbegin(), s.cend()) == 3);
    CHECK(s.lfront() == s.lbegin() && s.lend() == s.nil && s.contains(s.lfront()) && !s.contains(s.lend()));
    CHECK(s.find(4) == s.end() && s.count(4) == 0 && s.erase(4) == 0 && s.erase(s.lend()) == s.nil);
    CHECK(s.lower_bound(6) == s.end() && s.upper_bound(5) == s.end() && *s.upper_bound(0) == 1);
    const std::pair<typename S::iterator, bool> again = s.emplace(3);
    CHECK(!again.second && *again.first == 3 && s.size() == 3);
    S other;
    CHECK(other.erase(s.begin()) == other.end() && s.size() == 3 && !other.contains(s.begin()));
}

template <class M> void maps()
{
    M m;
    m.insert("b", 2);
    m.insert("a", 1);
    const M& cm  = m;
    int      sum = 0;
    CHECK(cm.with("a", [&](int const& v) { sum += v; }) && !cm.with("c", [](int const&) { CHECK(false); }));
    CHECK(cm.with(cm.lbegin(), [&](int const& v) { sum += v; }) && !cm.with(cm.lend(), [](int const&) { CHECK(false); }));
    cm.for_each([&](std::string const& k, int const& v) { sum += v + static_cast<int>(k.size()); });
    CHECK(sum == 7 && cm.cbegin()->key == "a" && std::distance(cm.cbegin(), cm.cend()) == 2);
    CHECK(cm.count("a") == 1 && cm.count("c") == 0 && cm.find("c") == cm.end() && cm.find("b")->value == 2);
    CHECK(cm.lower_bound("c") == cm.nil && cm.upper_bound("a") == cm.where("b") && cm.contains(cm.find("a")));
    CHECK(!cm.template key<std::optional>(cm.lend()) && !cm.template value<std::optional>(cm.lend()));
    CHECK(m.erase(m.find("a"))->key == "b" && m.erase(m.where("b")) == m.nil && m.empty() && m.erase("b") == 0);
}

void flats()
{
    cobflatset<int, 4> s;
    for (int x : { 5, 1, 3 }) {
        s.insert(x);
    }
    CHECK(std::distance(s.cbegin(), s.cend()) == 3 && *s.cbegin() == 1 && s.cend() == s.end());
    cobflatmap<std::string, int, 4> m;
    m.insert("b", 2);
    m.insert("a", 1);
    const auto& cm  = m;
    int         sum = 0;
    CHECK(cm.with("b", [&](int const& v) { sum += v; }) && !cm.with("c", [](int const&) { CHECK(false); }) && sum == 2);
    cm.for_each([&](std::string const&, int const& v) { sum += v; });
    CHECK(sum == 5 && cm.cbegin()->key == "a" && !m.with("c", [](int&) { CHECK(false); }));
    // ordering what it keeps is by the keys only
    const cobhlp::flatbykey<std::string, int, cobhlp::less<std::string>> by;
    const cobhlp::flatkeyval<std::string, int>                          a("a", 9), b("b", 1), none;
    CHECK(by(a, b) && !by(b, a) && by(a, std::string("b")) && by(std::string("a"), b) && none.key.empty() && none.value == 0);
    const cobhlp::bykey<std::string, int, cobhlp::less<std::string>> bylink;
    const cobhlp::keyval<std::string, int>                          ka("a", 9), kb("b", 1), knone;
    CHECK(bylink(ka, kb) && !bylink(kb, ka) && knone.key.empty() && knone.value == 0);
}

void hashes()
{
    cobhash<std::string, int, 4> h;
    h.insert("a", 1);
    h.insert("b", 2);
    const auto& ch  = h;
    int         sum = 0;
    CHECK(ch.with("a", [&](int const& v) { sum += v; }) && !ch.with("c", [](int const&) { CHECK(false); }));
    CHECK(!h.with("c", [](int&) { CHECK(false); }));
    ch.for_each([&](std::string const& k, int const& v) { sum += v + static_cast<int>(k.size()); });
    CHECK(sum == 6);
}

void vectors()
{
    cobvector<pt, 4> v;
    v.push_back(pt{ 1, 2 });
    v.push_back(pt{ 3, 4 });
    const auto& cv  = v;
    int         sum = 0;
    CHECK(cv.with(cobic<1>, [&](pt const& p) { sum += p.x; }) && !cv.with(cobic<2>, [](pt const&) { CHECK(false); }));
    CHECK(sum == 3 && cv.begin()->x == 1 && v.begin()->y == 2);

    cobvector<int, 6> n;
    const int         one = 1;
    auto              it  = n.insert(n.cend(), one);    // a copy, by an iterator
    CHECK(*it == 1 && n.size() == 1);
    n.insert(n.cbegin(), 0);
    n.push_back(2);
    it             = n.begin();
    const auto was = it++;
    CHECK(*was == 0 && *it == 1);
    const auto here = it--;
    CHECK(*here == 1 && *it == 0);
    it = n.end();
    it -= 1;
    CHECK(*it == 2 && (it - n.begin()) == 2);
    it -= 5;        // can't get out
    CHECK(it == n.begin() || it == n.end());
    auto       range = n.irange();
    auto       ri    = range.begin();
    const auto first = ri++;
    CHECK((*first).get() == 0 && (*ri).get() == 1);
    cobvector<int, 6> other;
    CHECK(other.insert(n.cbegin(), one) == other.end() && other.empty());   // of another vector
    CHECK(other.erase(n.cbegin()) == other.end() && other.erase(n.cbegin(), n.cend()) == other.end() && n.size() == 3);
}

struct nulify {
    bool operator()(int& id) const
    {
        if (id >= 0) {
            id = -1;
            return true;
        }
        return false;
    }
};

// What a mill does at the edges of its wheels
template <class M> void mills(bool lean)
{
    M          m;
    const auto top = M::max_duration;
    // a duration that no wheel has a spoke for starts nothing
    m.start(1, top + 1ms);
    m.start(2, -1ms);
    m.start(3, ms{ ms::max().count() });
    CHECK(m.stop(1) == -1 && m.stop(2) == -1 && m.stop(3) == -1);
    CHECK(!m.template stop_first<std::optional>().has_value());
    // the longest one does
    m.start(4, top);
    CHECK(m.stop(4) == 0 && m.stop(4) == -1);

    // the first one is looked for wheel by wheel
    m.start(10, ms{ 70 });      // on the second wheel
    CHECK(m.template stop_first<std::optional>() == 10 && !m.template stop_first<std::optional>().has_value());
    m.start(11, ms{ 3000 });    // on the third
    m.start(12, ms{ 70 });
    m.start(13, ms{ 5 });
    CHECK(m.template stop_first<std::optional>() == 13 && m.template stop_first<std::optional>() == 12);
    CHECK(m.template stop_first<std::optional>() == 11 && !m.template stop_first<std::optional>().has_value());
    // also when the first wheel is part of its way around
    int expired = 0;
    m.process_expired(ms{ 60 }, [&](int id) { expired += (id >= 0); });     // the lean mill reports "nothing"
    m.start(14, ms{ 70 });
    m.start(15, ms{ 2 });
    CHECK(m.template stop_first<std::optional>() == 15 && m.template stop_first<std::optional>() == 14 && expired == 0);

    // a timer that is stopped is not reported (in the lean mill it is there, as "nothing")
    const auto a = m.start(20, ms{ 4 });
    m.start(21, ms{ 4 });
    m.stop(a);
    std::vector<int> ids;
    m.process_expired(ms{ 200 }, [&](int id) { ids.push_back(id); });
    CHECK(std::count(ids.begin(), ids.end(), 21) == 1 && std::count(ids.begin(), ids.end(), 20) == 0);
    const auto nothings = static_cast<std::size_t>(std::count(ids.begin(), ids.end(), -1));
    CHECK(ids.size() == nothings + 1 && (lean ? (nothings >= 3) : (nothings == 0)));   // at least 14, 15 and 20
}

void timers()
{
    mills<cobtmrmill<int, 512, ms, 3>>(false);
    mills<cobcheckedtmrmill<int, 512, ms, 3>>(false);
    mills<cobtmrleanmill<int, 512, ms, nulify, 3>>(true);
    mills<cobcheckedtmrleanmill<int, 512, ms, nulify, 3>>(true);
    mills<cobtmrmill<int, 512, ms, 3, 64, true>>(false);

    // nothing to stop
    cobtmrlist<int, 4, ms> l;
    CHECK(l.empty() && l.stop(1) == -1 && !l.stop_first<std::optional>().has_value());
    l.process_expired(5ms, [](int) { CHECK(false); });
    const auto a = l.start(1, 5ms);
    CHECK(!l.empty() && l.stop(a) == 0 && l.stop(a) == -1 && l.empty());
    for (int i = 0; i < 4; ++i) {
        l.start(i, ms{ 10 - i });
    }
    CHECK(l.stop(l.start(9, 1ms)) == -1);      // no room: the index is of nothing
    CHECK(l.stop_first<std::optional>() == 3 && l.stop(0) == 0 && l.stop(0) == -1);
    std::vector<int> ids;
    l.process_expired(8ms, [&](int id) { ids.push_back(id); });
    CHECK((ids == std::vector<int>{ 2 }));
    l.process_expired(1ms, [&](int id) { ids.push_back(id); });
    CHECK((ids == std::vector<int>{ 2, 1 }) && l.empty());

    cobtmrwheel<int, 16, ms, 4> w;
    CHECK(w.stop(1) == -1 && !w.stop_first<std::optional>().has_value());
    w.start(1, 9ms);
    w.start(2, 2ms);
    w.start(3, 6ms);
    CHECK(w.stop_first<std::optional>() == 2 && w.stop(3) == 0 && w.stop(3) == -1);
    ids.clear();
    for (int t = 1; t <= 12; ++t) {
        w.process_expired(1ms, [&](int id) { ids.push_back(id * 100 + t); });
    }
    CHECK((ids == std::vector<int>{ 109 }));

    cobtmrleanring<int, 4, ms, nulify> r;
    CHECK(r.empty() && r.stop(1) == -1 && !r.stop_first<std::optional>().has_value());
    r.process_expired(5ms, [](int) { CHECK(false); });
    CHECK(r.start(1, 5ms) && r.start(2, 5ms) && r.start(3, 5ms) && r.start(4, 5ms));
    CHECK(!r.start(5, 5ms));                    // full: no room
    CHECK(r.stop(1) == 0 && r.stop(1) == -1);   // nullified in place, then gone
    CHECK(r.start(5, 5ms) && !r.start(6, 5ms)); // the hole is reused, then full again

    cobtmrleanring<int, 4, ms, nulify> h;
    h.start(1, 1ms); h.start(2, 2ms); h.start(3, 3ms);
    CHECK(h.stop(1) == 0);                      // a hole at the head
    CHECK(h.stop_first<std::optional>() == 2);  // skips the hole
    ids.clear();
    h.process_expired(3ms, [&](int id) { ids.push_back(id); });
    CHECK((ids == std::vector<int>{ 3 }) && h.empty());   // the cancelled one is never reported
    CHECK(!h.start(9, ms{ -1 }) && h.empty());            // a negative duration is not started
    CHECK(h.start(7, 0ms));
    ids.clear();
    h.process_expired(0ms, [&](int id) { ids.push_back(id); });
    CHECK((ids == std::vector<int>{ 7 }) && h.empty());   // zero expires on the next call
}

void queues()
{
    cobspsc<int, 2> q;
    cobspsc<int, 2> other;
    auto            tx  = q.claim_producer<std::optional>();
    auto            rx  = q.claim_consumer<std::optional>();
    auto            otx = other.claim_producer<std::optional>();
    auto            orx = other.claim_consumer<std::optional>();
    CHECK(tx && rx && otx && orx);
    // a side of another queue is not given back to this one
    CHECK(!q.release(std::move(*otx)) && !q.release(std::move(*orx)));
    CHECK(otx->push(1) && orx->pop<std::optional>() == 1 && !other.claim_producer<std::optional>());
    // assigned over, a side is that of the other queue, and what it was assigned from is of none
    CHECK(tx->push(5));
    *orx = std::move(*rx);
    *otx = std::move(*tx);
    CHECK(orx->pop<std::optional>() == 5 && otx->push(6) && q.size() == 1 && other.empty());
    CHECK(!rx->pop<std::optional>() && rx->empty() && !rx->drop_front() && !rx->with_front([](int&) {}));
    CHECK(!tx->push(7) && tx->full() && !q.release(std::move(*tx)) && !q.release(std::move(*rx)));
    // and can be given back to the queue it is of
    CHECK(!other.release(std::move(*otx)) && q.release(std::move(*otx)) && q.claim_producer<std::optional>());
    CHECK(orx->with_front([](int& x) { CHECK(x == 6); }) && orx->drop_front() && !orx->drop_front() && q.empty());
    CHECK(q.release(std::move(*orx)) && q.claim_consumer<std::optional>() && !q.claim_consumer<std::optional>());
}

// What is done with what is not there, by all the kinds of access
void optionals()
{
    perhaps<std::string>       none;
    const perhaps<std::string> cnone;
    perhaps<std::string>       some("abc");
    const perhaps<std::string> csome("abcd");
    const auto                 size = [](std::string const& s) { return s.size(); };
    CHECK(none.and_then(size) == 0 && cnone.and_then(size) == 0 && some.and_then(size) == 3 && csome.and_then(size) == 4);
    CHECK(perhaps<std::string>().and_then(size) == 0 && perhaps<std::string>("ab").and_then(size) == 2);
    CHECK(std::move(cnone).and_then(size) == 0 && std::move(csome).and_then(size) == 4);
    int calls = 0;
    const auto count = [&](std::string const&) { ++calls; };
    none.and_then(count);
    cnone.and_then(count);
    perhaps<std::string>().and_then(count);
    std::move(cnone).and_then(count);
    CHECK(calls == 0);
    some.and_then(count);
    csome.and_then(count);
    perhaps<std::string>("x").and_then(count);
    std::move(csome).and_then(count);
    CHECK(calls == 4);
    CHECK(none.value_or("z") == "z" && perhaps<std::string>("y").value_or("z") == "y" && some.value_or("z") == "abc");
    CHECK(!none.transform(size).has_value() && some.transform(size).value_or(0) == 3);
    CHECK(none.or_else([] { return perhaps<std::string>("e"); }).value_or("") == "e");
    CHECK(perhaps<std::string>().or_else([] { return perhaps<std::string>("e"); }).value_or("") == "e");
    CHECK(some.or_else([] { return perhaps<std::string>("e"); }).value_or("") == "abc");
    none = some;
    CHECK(none == some && !(none == cnone) && cnone == perhaps<std::string>());
    none = cnone;
    CHECK(!none.has_value());
    none = perhaps<std::string>("moved");
    some = std::move(none);
    CHECK(some.value_or("") == "moved");
    some.emplace(3u, 'k');
    CHECK(some.value_or("") == "kkk");
    some.reset();
    some.reset();
    CHECK(!some.has_value() && perhaps<int>::unit(5).value_or(0) == 5);

    contingent<int, nothing_is<-1>> c;
    calls = 0;
    c.and_then([&](int) { ++calls; });
    CHECK(calls == 0 && c.and_then([](int x) { return x + 1; }) == 0 && !c.transform([](int x) { return x; }).has_value());
    c = 4;
    c.and_then([&](int) { ++calls; });
    CHECK(calls == 1 && c.and_then([](int x) { return x + 1; }) == 5 && c.transform([](int x) { return x * 2; }).value_or(0) == 8);
    CHECK(c.or_else([] { return contingent<int, nothing_is<-1>>(9); }).value_or(0) == 4);
    c.reset();
    CHECK(c.or_else([] { return contingent<int, nothing_is<-1>>(9); }).value_or(0) == 9 && c.value_or(7) == 7);
    CHECK(c == (contingent<int, nothing_is<-1>>(-1)) && !(c == contingent<int, nothing_is<-1>>(0)));
    contingent<float> f;
    CHECK(!f.has_value() && contingent<float>(1.5f).value_or(0.f) == 1.5f && f.value_or(2.f) == 2.f);
    contingent<cobint<0, 5>> ci;
    CHECK(!ci.has_value() && (contingent<cobint<0, 5>>(cobic<3>).value_or(cobic<0>).get() == 3));

    int               x = 1;
    optptr<int>       op(&x);
    const optptr<int> cop(&x);
    optptr<int>       onull(nullptr);
    calls = 0;
    op.and_then([&](int& v) { ++v; ++calls; });
    cop.and_then([&](int const&) { ++calls; });
    onull.and_then([&](int&) { ++calls; });
    CHECK(calls == 2 && x == 2 && op.and_then([](int& v) { return v; }) == 2 && onull.and_then([](int& v) { return v; }) == 0);
    CHECK(cop.and_then([](int const& v) { return v; }) == 2 && static_cast<bool>(op) && !onull);
    int y = 5;
    CHECK(*onull.value_or(fullptr<int>::from(y)) == 5 && *op.value_or(fullptr<int>::from(y)) == 2);
    CHECK(static_cast<bool>(onull.or_else([&] { return optptr<int>(&y); })) && op.release() == &x && !op);
    onull.reset();
    CHECK(!onull);

    auto os = optshared<std::string>(std::make_shared<std::string>("s"));
    optshared<std::string> osnull(nullptr);
    calls = 0;
    os.and_then([&](std::string& s) { s += "t"; ++calls; });
    osnull.and_then([&](std::string&) { ++calls; });
    CHECK(calls == 1 && os.and_then(size) == 2 && osnull.and_then(size) == 0 && os && !osnull);
    CHECK(*osnull.value_or(fullshared<std::string>::make("d")) == "d" && os.full<std::optional>().has_value());
    CHECK(!osnull.full<std::optional>().has_value() && osnull.or_else([&] { return os; }).shared() == os.shared());
    os.reset();
    CHECK(!os && !fullshared<std::string>::from<std::optional>(nullptr).has_value());

    auto ou = optunique<std::string>::make("u");
    optunique<std::string> ounull(nullptr);
    calls = 0;
    ou.and_then([&](std::string& s) { s += "v"; ++calls; });
    ounull.and_then([&](std::string&) { ++calls; });
    const auto& cou = ou;
    CHECK(calls == 1 && cou.and_then(size) == 2 && ounull.and_then(size) == 0 && ou && !ounull);
    cou.and_then([&](std::string const&) { ++calls; });
    ounull = std::move(ou);
    CHECK(calls == 2 && ounull && *ounull.release() == "uv" && !ounull);
}

int main()
{
    CHECK(atbdim<four>::set(4) && atbdim<wide>::set(100));
    arrays();
    matrices();
    bitsets();
    strings();
    lists();
    sets<cobskiplist<int, 8>>();
    sets<cobcheckedskiplist<int, 8>>();
    sets<cobtree<int, 8>>();
    sets<cobcheckedtree<int, 8>>();
    maps<cobmap<std::string, int, 4>>();
    maps<cobtreemap<std::string, int, 4>>();
    maps<cobcheckedmap<std::string, int, 4>>();
    flats();
    hashes();
    vectors();
    timers();
    queues();
    optionals();
    return 0;
}
