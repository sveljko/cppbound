#include "atbbinheap.hpp"
#include "atbbitset.hpp"
#include "atbfwdlist.hpp"
#include "atbhash.hpp"
#include "atblist.hpp"
#include "atbmap.hpp"
#include "atbmatrix.hpp"
#include "atbpool.hpp"
#include "atbring.hpp"
#include "atbskiplist.hpp"
#include "atbstring.hpp"
#include "atbvector.hpp"

#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <algorithm>
#include <bitset>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <ranges>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// A dimension can be set only once per tag per program
template <int K> struct tg {};
struct unset {};        // the dimension of this one is never set, so it is 0

using dim   = tg<0>;    // set to DIM
using rows  = tg<1>;
using cols  = tg<2>;
using big   = tg<3>;    // more than a string can take, and than a word of bits
using one   = tg<4>;

constexpr std::size_t DIM = 37;

static_assert(std::bidirectional_iterator<atblist<int, dim>::I>);
static_assert(std::ranges::bidirectional_range<atblist<int, dim> const>);
static_assert(std::forward_iterator<atbfwdlist<int, dim>::CI>);
static_assert(std::forward_iterator<atbskiplist<int, dim>::CI>);
static_assert(std::random_access_iterator<atbvector<int, dim>::I>);
static_assert(std::random_access_iterator<atbmatrix<int, rows, cols>::I>);
static_assert(std::ranges::random_access_range<atbmatrix<int, rows, cols> const>);
// No literal is known to fit, at compile time
static_assert(!std::is_constructible_v<atbstring<dim>, char const (&)[3]>);
// An index of a vector, a bit or a character is made by a checked function only
static_assert(std::is_same_v<atbvector<int, dim>::index, atbidx<dim>>);
static_assert(std::is_same_v<atbbitset<dim>::index, atbidx<dim>>);
static_assert(std::is_same_v<atbstring<dim>::index, atbidx<dim>>);

template <class Tag> atbidx<Tag> at(std::size_t i)
{
    auto idx = atbidx<Tag>::template be<std::optional>(i);
    CHECK(idx.has_value());
    return *idx;
}

template <class L, class M> bool same(L const& l, M const& model)
{
    return std::equal(l.begin(), l.end(), model.begin(), model.end())
           && (static_cast<std::size_t>(l.size()) == static_cast<std::size_t>(std::distance(model.begin(), model.end())));
}

void pool()
{
    atbpool<std::string, dim> p;
    CHECK(p.empty() && p.max_size() == DIM);
    std::mt19937                                                        rng(1);
    std::vector<std::pair<atbpool<std::string, dim>::handle, std::string>> live;
    for (int step = 0; step < 5000; ++step) {
        if (live.empty() || (rng() % 3 != 0)) {
            const std::string v = std::to_string(step);
            const auto        h = p.make(v);
            CHECK((h != p.nil) == (live.size() < DIM));
            if (h != p.nil) {
                live.push_back({ h, v });
            }
        }
        else {
            const std::size_t i = rng() % live.size();
            CHECK(p.get(live[i].first) == live[i].second);
            CHECK(p.drop(live[i].first) && !p.drop(live[i].first) && !p.live(live[i].first));
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(i));
        }
        CHECK(p.size() == live.size() && p.full() == (live.size() == DIM));
    }
    for (auto const& [h, v] : live) {
        CHECK(p.live(h) && p.get(h) == v);
    }
    // a copy is a pool of its own, where the same handles work; so is what is "moved" from
    auto c = p;
    auto m = std::move(p);
    for (auto const& [h, v] : live) {
        CHECK(c.get(h) == v && m.get(h) == v && p.get(h) == v);
    }
    c.clear();
    CHECK(c.empty() && p.size() == live.size());
    CHECK(!p.drop(p.nil) && !p.live(p.nil));
}

void lists()
{
    std::mt19937 rng(2);
    {
        atblist<int, dim> l;
        std::list<int>    model;
        CHECK(l.empty() && l.max_size() == DIM && l.lbegin() == l.lend() && l.lend() == l.nil);
        for (int step = 0; step < 6000; ++step) {
            const int x = static_cast<int>(rng() % 50);
            switch (rng() % 8) {
            case 0: CHECK(l.push_back(x) == (model.size() < DIM)); if (model.size() < DIM) model.push_back(x); break;
            case 1: CHECK(l.push_front(x) == (model.size() < DIM)); if (model.size() < DIM) model.push_front(x); break;
            case 2: {
                auto v = l.pop_front<std::optional>();
                CHECK(v.has_value() == !model.empty());
                if (v) { CHECK(*v == model.front()); model.pop_front(); }
                break;
            }
            case 3: {
                auto v = l.pop_back<std::optional>();
                CHECK(v.has_value() == !model.empty());
                if (v) { CHECK(*v == model.back()); model.pop_back(); }
                break;
            }
            case 4: {   // somewhere in the middle
                const std::size_t skip = model.empty() ? 0 : rng() % model.size();
                auto              pos  = l.lbegin();
                for (std::size_t i = 0; i < skip; ++i) { pos = l.fwd(pos); }
                const auto at_ = l.insert(pos, x);
                CHECK((at_ != l.nil) == (model.size() < DIM));
                if (at_ != l.nil) { model.insert(std::next(model.begin(), static_cast<std::ptrdiff_t>(skip)), x); CHECK(l.get(at_) == x); }
                break;
            }
            case 5: {
                if (!model.empty()) {
                    const std::size_t skip = rng() % model.size();
                    auto              pos  = l.lbegin();
                    for (std::size_t i = 0; i < skip; ++i) { pos = l.fwd(pos); }
                    l.erase(pos);
                    CHECK(!l.live(pos) && l.erase(pos) == l.nil);
                    model.erase(std::next(model.begin(), static_cast<std::ptrdiff_t>(skip)));
                }
                break;
            }
            case 6: CHECK(l.remove(x) == model.remove(x)); break;
            default:
                if (rng() % 8 == 0) { l.sort(); model.sort(); CHECK(l.unique() == model.unique()); }
                else { l.reverse(); model.reverse(); }
            }
            CHECK(same(l, model));
            CHECK(l.front<std::optional>().has_value() == !model.empty());
        }
        CHECK(std::equal(std::make_reverse_iterator(l.end()), std::make_reverse_iterator(l.begin()), model.rbegin(), model.rend()));

        atblist<int, dim> other;
        other.push_back(1000);
        other.push_back(1001);
        l.clear();
        model.clear();
        for (int x : { 5, 1, 9 }) { l.push_back(x); }
        CHECK(l.splice(l.lbegin(), other) && other.empty() && same(l, std::list<int>{ 1000, 1001, 5, 1, 9 }));
        auto copy = l;
        l.sort();
        CHECK(same(l, std::list<int>{ 1, 5, 9, 1000, 1001 }) && same(copy, std::list<int>{ 1000, 1001, 5, 1, 9 }));
        other = l;
        CHECK(!l.merge(copy) || (DIM >= 10));
        atblist<int, dim>::I none;
        CHECK(*none == 0);      // an iterator to nowhere is harmless
    }
    {
        atbfwdlist<std::string, dim> l;
        std::forward_list<std::string> model;
        std::size_t                    n = 0;
        for (int step = 0; step < 4000; ++step) {
            const std::string x(1 + rng() % 3, static_cast<char>('a' + rng() % 5));
            switch (rng() % 6) {
            case 0:
            case 1: CHECK(l.push_front(x) == (n < DIM)); if (n < DIM) { model.push_front(x); ++n; } break;
            case 2: {
                auto v = l.pop_front<std::optional>();
                CHECK(v.has_value() == (n > 0));
                if (v) { CHECK(*v == model.front()); model.pop_front(); --n; }
                break;
            }
            case 3:
                if (n > 0) {
                    CHECK((l.insert_after(l.lbegin(), x) != l.nil) == (n < DIM));
                    if (n < DIM) { model.insert_after(model.begin(), x); ++n; }
                    if (n > 1 && rng() % 2) { l.erase_after(l.lbegin()); model.erase_after(model.begin()); --n; }
                }
                break;
            case 4: { const auto gone = l.remove(x); CHECK(gone == model.remove(x)); n -= gone; break; }
            default:
                if (rng() % 4 == 0) { l.sort(); model.sort(); const auto gone = l.unique(); CHECK(gone == model.unique()); n -= gone; }
                else { l.reverse(); model.reverse(); }
            }
            CHECK(l.size() == n && std::equal(l.begin(), l.end(), model.begin(), model.end()));
        }
        auto copy = l;
        l.clear();
        CHECK(l.empty() && copy.size() == n && std::equal(copy.begin(), copy.end(), model.begin(), model.end()));
    }
}

void skiplists()
{
    std::mt19937 rng(3);
    {
        atbskiplist<int, dim> s;
        std::set<int>         model;
        for (int step = 0; step < 8000; ++step) {
            const int x = static_cast<int>(rng() % 80);
            if (rng() % 5 < 3) {
                const bool should = (model.size() < DIM) && !model.count(x);
                CHECK(s.insert(x).second == should);
                if (should) model.insert(x);
            }
            else {
                CHECK(s.erase(x) == model.erase(x));
            }
            CHECK(s.count(x) == model.count(x) && s.size() == model.size());
            const auto lb = model.lower_bound(x);
            CHECK((lb == model.end()) ? (s.lower_bound(x) == s.end()) : (*s.lower_bound(x) == *lb));
        }
        CHECK(same(s, model));
        auto copy = s;
        s.clear();
        CHECK(s.empty() && same(copy, model));
    }
    {
        atbmap<std::string, int, dim> m;
        std::map<std::string, int>    model;
        for (int step = 0; step < 8000; ++step) {
            const std::string k = std::to_string(rng() % 60);
            const int         v = static_cast<int>(rng() % 1000);
            switch (rng() % 4) {
            case 0: {
                const bool should = (model.size() < DIM) && !model.count(k);
                CHECK(m.insert(k, v).second == should);
                if (should) model[k] = v;
                break;
            }
            case 1:
                if (model.count(k) || (model.size() < DIM)) {
                    CHECK(m.insert_or_assign(k, v).first != m.nil);
                    model[k] = v;
                }
                break;
            case 2: CHECK(m.erase(k) == model.erase(k)); break;
            default: CHECK(m.with(k, [&](int& x) { x = v; }) == (model.count(k) > 0)); if (model.count(k)) model[k] = v;
            }
            CHECK(m.size() == model.size() && m.contains(k) == (model.count(k) > 0));
            if (model.count(k)) CHECK(m.find<std::optional>(k) == model[k]);
        }
        auto it = model.begin();
        m.for_each([&](std::string const& k, int const& v) {
            CHECK(it != model.end() && it->first == k && it->second == v);
            ++it;
        });
        CHECK(it == model.end());
    }
}

void vectors()
{
    std::mt19937 rng(4);
    atbvector<std::string, dim> v;
    std::vector<std::string>    model;
    CHECK(v.empty() && v.capacity() == DIM);
    for (int step = 0; step < 6000; ++step) {
        const std::string x   = std::to_string(rng() % 100);
        const std::size_t pos = rng() % (DIM + 2);
        const auto        idx = atbidx<dim>::be<std::optional>(pos);
        switch (rng() % 6) {
        case 0:
        case 1: CHECK(v.push_back(x) == (model.size() < DIM)); if (model.size() < DIM) model.push_back(x); break;
        case 2: {
            atbvector<std::string, dim>::position p;
            if (p.be(pos)) {
                const bool ok = (pos <= model.size()) && (model.size() < DIM);
                CHECK(v.insert(p, x) == ok);
                if (ok) model.insert(model.begin() + static_cast<std::ptrdiff_t>(pos), x);
            }
            break;
        }
        case 3:
            if (idx) {
                CHECK(v.erase(*idx) == (pos < model.size()));
                if (pos < model.size()) model.erase(model.begin() + static_cast<std::ptrdiff_t>(pos));
            }
            break;
        case 4: {
            auto b = v.pop_back<std::optional>();
            CHECK(b.has_value() == !model.empty());
            if (b) { CHECK(*b == model.back()); model.pop_back(); }
            break;
        }
        default:
            if (idx) {
                CHECK(v.set(*idx, x) == (pos < model.size()) && v.live(*idx) == (pos < model.size()));
                if (pos < model.size()) model[pos] = x;
                CHECK(v.at<std::optional>(*idx).has_value() == (pos < model.size()));
            }
        }
        std::vector<std::string> got;
        for (auto i : v.irange()) { got.push_back(v.get(i)); }
        CHECK(got == model && v.size() == model.size());
    }
    std::sort(v.begin(), v.end());
    std::sort(model.begin(), model.end());
    CHECK(std::equal(v.begin(), v.end(), model.begin(), model.end()) && v.begin() + (DIM + 5) == v.end());
    if (!model.empty()) {
        const auto after = v.erase(v.begin());
        CHECK(after == v.begin());
        model.erase(model.begin());
    }
    atbvector<std::string, dim>::CI nowhere;
    CHECK(nowhere->empty());
    auto copy = v;
    CHECK(copy == v);
    v.clear();
    CHECK(v.empty() && copy.size() == model.size() && !(copy == v) == !model.empty());

    atbbinheap<int, dim>     h;
    std::priority_queue<int> pq;
    for (int step = 0; step < 6000; ++step) {
        if (rng() % 5 < 3) {
            const int x = static_cast<int>(rng() % 100);
            CHECK(h.push(x) == (pq.size() < DIM));
            if (pq.size() < DIM) pq.push(x);
        }
        else {
            auto t = h.pop<std::optional>();
            CHECK(t.has_value() == !pq.empty());
            if (t) { CHECK(*t == pq.top()); pq.pop(); }
        }
        CHECK(h.size() == pq.size() && (pq.empty() || h.top<std::optional>() == pq.top()));
    }
}

void strings()
{
    atbstring<dim> s;
    CHECK(s.empty() && s.capacity() == DIM && s.view().data()[0] == '\0');
    CHECK(static_cast<unsigned char>(s.view().data()[DIM]) == DIM);
    CHECK(s.assign("hello") && s.append(", world") && s == "hello, world" && s.size() == 12);
    CHECK(static_cast<unsigned char>(s.view().data()[DIM]) == DIM - 12);
    CHECK(!s.append(DIM, 'x') && s.append(DIM - 12, 'x') && s.full() && !s.push_back('y'));
    CHECK(s.view().data()[DIM] == '\0' && std::string(s.view().data()).size() == DIM);
    auto comma = s.find<std::optional>(',');
    CHECK(comma.has_value() && comma->get() == 5 && s.get(*comma) == ',' && s.set(*comma, ';'));
    CHECK(s.erase(5) && s == "hello" && s.get(at<dim>(5)) == '\0' && !s.set(at<dim>(5), 'x'));
    CHECK(s.insert(0, s.view()) && s.replace(0, 5, "HE") && s == "HEhello" && s.substr(2) == "hello");
    auto copy = s;
    CHECK(copy == s && copy.resize(2) && copy == "HE" && s == "HEhello" && copy < s);
    CHECK(atbstring<dim>::from<std::optional>("fits").has_value());
    CHECK(!atbstring<dim>::from<std::optional>(std::string(DIM + 1, 'x')).has_value());

    // no more than 255 characters, whatever the dimension
    atbstring<big> b;
    CHECK(b.capacity() == 255 && b.assign(255, 'b') && b.full() && !b.push_back('c') && b.view().data()[255] == '\0');
    CHECK(s.assign("bb") && s < b && b > s && !(b == s) && b.starts_with(s));

    atbstring<one> o;
    CHECK(o.capacity() == 1 && o.push_back('a') && o.full() && !o.push_back('b') && o == "a");
    CHECK(o.pop_back<std::optional>() == 'a' && o.empty());
}

void rings_bits_hashes()
{
    std::mt19937 rng(5);
    atbring<std::string, dim> r;
    std::deque<std::string>   model;
    for (int step = 0; step < 5000; ++step) {
        const std::string x = std::to_string(step);
        if (rng() % 5 < 3) {
            CHECK(r.push_back(x) == (model.size() < DIM));
            if (model.size() < DIM) model.push_back(x);
        }
        else if (rng() % 7 == 0) {
            r.emplace_over(x);
            if (model.size() == DIM) model.pop_front();
            model.push_back(x);
        }
        else {
            auto f = r.pop_front<std::optional>();
            CHECK(f.has_value() == !model.empty());
            if (f) { CHECK(*f == model.front()); model.pop_front(); }
        }
        CHECK(r.size() == model.size());
        if (!model.empty()) {
            CHECK(r.front<std::optional>() == model.front() && r.back<std::optional>() == model.back());
            CHECK(r.at<std::optional>(model.size() / 2) == model[model.size() / 2]);
        }
        CHECK(!r.at<std::optional>(model.size()).has_value());
    }
    auto rc = r;
    r.clear();
    CHECK(r.empty() && rc.size() == model.size());

    constexpr std::size_t BITS = 300;
    atbbitset<big>        b;
    std::bitset<BITS>     bm;
    CHECK(b.size() == BITS && b.none() && !b.any() && !b.first<std::optional>().has_value());
    for (int step = 0; step < 3000; ++step) {
        const std::size_t i = rng() % BITS;
        switch (rng() % 4) {
        case 0: b.set(at<big>(i)); bm.set(i); break;
        case 1: b.reset(at<big>(i)); bm.reset(i); break;
        case 2: b.flip(at<big>(i)); bm.flip(i); break;
        default: CHECK(b.maybe_set(i, i % 2) && !b.maybe_set(BITS + i) && !b.maybe_set(-1)); bm.set(i, i % 2);
        }
        CHECK(b.test(at<big>(i)) == bm.test(i) && b.count() == bm.count());
    }
    std::size_t seen = 0;
    auto        bit  = b.first<std::optional>();
    b.for_each([&](atbidx<big> i) {
        CHECK(bm.test(i.get()) && bit.has_value() && *bit == i);
        bit = b.next<std::optional>(i);
        ++seen;
    });
    CHECK(seen == bm.count() && !bit.has_value());
    auto all = b | ~b;
    CHECK(all.all() && all.count() == BITS && (b & ~b).none() && (b ^ b).none() && (~~b == b));
    all.reset();
    CHECK(all.none());
    all.set();
    CHECK(all.all());

    atbhash<int, std::string, dim>       h;
    std::unordered_map<int, std::string> hm;
    for (int step = 0; step < 8000; ++step) {
        const int         k = static_cast<int>(rng() % 90) * 37;
        const std::string v = std::to_string(step);
        switch (rng() % 3) {
        case 0: {
            const bool should = (hm.size() < DIM) && !hm.count(k);
            CHECK(h.insert(k, v) == should);
            if (should) hm[k] = v;
            break;
        }
        case 1: CHECK(h.erase(k) == (hm.erase(k) > 0)); break;
        default:
            if (hm.count(k) || (hm.size() < DIM)) {
                CHECK(h.insert_or_assign(k, v));
                hm[k] = v;
            }
        }
        CHECK(h.size() == hm.size() && h.contains(k) == (hm.count(k) > 0));
        if (hm.count(k)) CHECK(h.find<std::optional>(k) == hm[k]);
    }
    std::size_t entries = 0;
    h.for_each([&](int const& k, std::string const& v) {
        CHECK(hm.at(k) == v);
        ++entries;
    });
    CHECK(entries == hm.size());
}

void matrices()
{
    using matrix = atbmatrix<int, rows, cols>;
    matrix m;
    CHECK(m.rows() == 4 && m.columns() == 3 && m.size() == 12 && !m.empty());
    m.fill(0);
    for (auto r : atbidx<rows>::range()) {
        for (auto c : atbidx<cols>::range()) {
            m.set({ r, c }, static_cast<int>(r.get() * 10 + c.get()));
        }
    }
    CHECK(m.get({ at<rows>(2), at<cols>(1) }) == 21 && m.front() == 0 && m.back() == 32);
    std::vector<int> all(m.begin(), m.end());
    CHECK((all == std::vector<int>{ 0, 1, 2, 10, 11, 12, 20, 21, 22, 30, 31, 32 }));
    CHECK(m.end() - m.begin() == 12 && *(m.begin() + 4) == 11 && m.begin()[11] == 32);
    CHECK(m.begin() + 100 == m.end() && m.begin() - 1 == m.end());     // out of bounds: the safety spot
    CHECK(m.maybe_set(3, 2, 99) && !m.maybe_set(4, 0, 1) && !m.maybe_set(0, 3, 1) && !m.maybe_set(-1, 0, 1));
    CHECK(m.back() == 99);

    auto row = m.get_row(at<rows>(1));
    CHECK(row.size() == 3 && row.get(at<cols>(2)) == 12);
    row.set(at<cols>(0), 77);
    m.set_row(at<rows>(0), row);
    CHECK(m.get({ at<rows>(0), at<cols>(0) }) == 77 && m.get({ at<rows>(0), at<cols>(2) }) == 12);

    auto gen = matrix::generate([](matrix::index i) { return static_cast<int>(i.irow.get() + i.icolumn.get()); });
    CHECK(gen.get({ at<rows>(3), at<cols>(2) }) == 5);
    auto copy = m;
    CHECK(copy == m && !(copy == gen));
    copy.swap(gen);
    CHECK(gen == m && matrix::filled(3).back() == 3);
    std::ranges::sort(m);
    CHECK(std::ranges::is_sorted(m));
    matrix::I none;
    CHECK(none != m.begin());

    struct S {
        int a = 0;
        int b = 0;
    };
    atbmatrix<S, rows, cols> sm;
    sm.grab({ at<rows>(1), at<cols>(1) })->a = 5;
    sm.set({ at<rows>(1), at<cols>(1) }, &S::b, 6);
    CHECK(sm.get({ at<rows>(1), at<cols>(1) }, &S::a) == 5 && sm.grab({ at<rows>(1), at<cols>(1) })->b == 6);
}

// With the dimension never set, there is no room for anything, and no index
void nothing()
{
    atbpool<int, unset> p;
    CHECK(p.make(1) == p.nil && p.full() && p.empty() && p.max_size() == 0);
    atblist<int, unset> l;
    CHECK(!l.push_back(1) && !l.push_front(1) && l.insert(l.lend(), 1) == l.nil && l.empty() && l.begin() == l.end());
    CHECK(!l.pop_front<std::optional>().has_value() && l.get(l.lbegin()) == 0);
    l.sort();
    l.reverse();
    atbfwdlist<int, unset> f;
    CHECK(!f.push_front(1) && f.empty() && f.remove(1) == 0);
    atbskiplist<int, unset> s;
    CHECK(!s.insert(1).second && s.empty() && s.count(1) == 0 && s.erase(1) == 0 && s.find(1) == s.end());
    atbmap<int, int, unset> m;
    CHECK(!m.insert(1, 1).second && !m.contains(1) && m.erase(1) == 0 && m.insert_or_assign(1, 1).first == m.nil);
    atbvector<int, unset> v;
    CHECK(!v.push_back(1) && v.full() && v.empty() && !v.pop_back<std::optional>().has_value() && v.resize(0) && !v.resize(1));
    atbbinheap<int, unset> h;
    CHECK(!h.push(1) && !h.pop<std::optional>().has_value());
    atbstring<unset> str;
    CHECK(str.capacity() == 0 && str.empty() && str.full() && !str.push_back('a') && str.assign("") && !str.assign("a"));
    CHECK(str.view().data()[0] == '\0' && str == "" && str.erase() && str.substr().empty());
    atbring<int, unset> r;
    CHECK(!r.push_back(1) && r.empty() && r.full() && !r.pop_front<std::optional>().has_value());
    atbbitset<unset> b;
    CHECK(b.count() == 0 && b.none() && b.all() && !b.maybe_set(0) && !b.first<std::optional>().has_value());
    b.set();
    b.flip();
    CHECK(b.none());
    atbhash<int, int, unset> hh;
    CHECK(!hh.insert(1, 1) && !hh.contains(1) && !hh.erase(1) && !hh.insert_or_assign(1, 1) && !hh.find<std::optional>(1).has_value());
    atbmatrix<int, unset, cols> mx;
    CHECK(mx.empty() && mx.size() == 0 && mx.begin() == mx.end() && !mx.maybe_set(0, 0, 1));
    mx.fill(1);
    CHECK(!atbidx<unset>::be<std::optional>(0).has_value());
}

// What is put in is constructed then, and destroyed when taken out
void lifetimes()
{
    counted::alive = 0;
    {
        atblist<counted, dim>   l;
        atbvector<counted, dim> v;
        atbring<counted, dim>   r;
        atbpool<counted, dim>   p;
        atbmap<int, counted, dim> m;
        CHECK(counted::alive == 0);
        for (int i = 0; i < 5; ++i) {
            l.emplace_back(i);
            v.emplace_back(i);
            r.emplace_back(i);
            p.make(i);
            m.emplace(i, i);
        }
        CHECK(counted::alive == 25);
        auto l2 = l;
        auto v2 = v;
        auto r2 = r;
        auto p2 = p;
        auto m2 = m;
        CHECK(counted::alive == 50);
        l.clear();
        v.clear();
        r.clear();
        p.clear();
        m.clear();
        CHECK(counted::alive == 25);
        l = l2;
        CHECK(counted::alive == 30 && l.size() == 5);
    }
    CHECK(counted::alive == 0);

    atblist<std::unique_ptr<int>, dim> ul;
    ul.emplace_back(new int(2));
    CHECK(ul.with(ul.lfront(), [](std::unique_ptr<int>& p) { *p = 3; }));
    atbvector<nodefault, dim> nd;
    CHECK(nd.emplace_back(4));
}

int main(int argc, char**)
{
    // Nothing of a tag may be used before its dimension is set
    CHECK(atbdim<dim>::set(DIM + static_cast<std::size_t>(argc) - 1));
    CHECK(atbdim<rows>::set(4) && atbdim<cols>::set(3) && atbdim<big>::set(300) && atbdim<one>::set(1));

    pool();
    lists();
    skiplists();
    vectors();
    strings();
    rings_bits_hashes();
    matrices();
    nothing();
    lifetimes();

    // Too late now
    CHECK(!atbdim<dim>::set(5) && !atbdim<unset>::set(5));
    return 0;
}
