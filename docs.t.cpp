// The claims and examples of the documentation (*.md), as code: if this
// fails, a document says something that the code does not do.

#include "atbarray.hpp"
#include "atbfunction.hpp"
#include "atblist.hpp"
#include "atbmatrix.hpp"
#include "atbstring.hpp"
#include "cobarray.hpp"
#include "cobbinheap.hpp"
#include "cobbitset.hpp"
#include "cobflatmap.hpp"
#include "cobflatset.hpp"
#include "cobfunction.hpp"
#include "cobfwdlist.hpp"
#include "cobhash.hpp"
#include "coblist.hpp"
#include "coblru.hpp"
#include "cobmap.hpp"
#include "cobmatrix.hpp"
#include "cobpool.hpp"
#include "cobring.hpp"
#include "cobskiplist.hpp"
#include "cobspsc.hpp"
#include "cobstring.hpp"
#include "cobtmrleanmill.hpp"
#include "cobtmrlist.hpp"
#include "cobtmrmill.hpp"
#include "cobtmrwheel.hpp"
#include "cobtree.hpp"
#include "cobtreemap.hpp"
#include "cobvector.hpp"
#include "contingent.hpp"
#include "fullptr.hpp"
#include "fullshared.hpp"
#include "fullunique.hpp"
#include "optptr.hpp"
#include "optshared.hpp"
#include "optunique.hpp"
#include "perhaps.hpp"
#include "cobtesthelp.hpp"
#include <chrono>
#include <optional>
#include <sstream>
#include <string>
using ms = std::chrono::milliseconds;
template <class U> struct ignore { ignore() {} ignore(U const&) {} };
struct sessions {}; struct points {}; using edges = atbpart<points, 3>;
struct bytes {};
struct S { int i; float f = 0.0; };
struct cell { int value = 0; bool seen = false; };
struct my_id { int v = -1; constexpr bool valid() const { return v >= 0; } static constexpr my_id invalid() { return {}; } bool operator==(my_id const&) const = default; };
template <> struct nothing<my_id> {
    using stored = my_id;
    static constexpr my_id        none() noexcept { return my_id::invalid(); }
    static constexpr bool         is_none(my_id const& x) noexcept { return !x.valid(); }
    static constexpr my_id        pack(my_id x) { return x; }
    static constexpr my_id const& unpack(my_id const& s) { return s; }
};
template <class C> concept has_first = requires(C c) { c.first(); };
template <class A, class B> concept constructs = std::is_constructible_v<A, B>;
int main()
{
    // common.md
    { coblist<int, 4> list; list.push_back(1); auto x = list.front<std::optional>(); CHECK(x == 1); list.pop_front<ignore>(); CHECK(list.empty());
      cobcheckedlist<int, 64> l; auto a = l.insert(l.lend(), 1); l.erase(a); auto b = l.insert(l.lend(), 2); CHECK(!l.live(a) && l.live(b));
      static_assert(std::is_same_v<coblist<int, 8>::link, cobint<0, 8>>);
      coblist<int, 4>::iterator none; CHECK(*none == 0); }
    // cobpool.md
    { cobhive<int, 8> p; auto a = p.make(1); auto b = p.make(2); p.drop(a); auto c = p.make(3); CHECK(c == a);     // freed last, used first
      int n = 0; for (auto h = p.first(); h != p.nil; h = p.next(h)) { p.with(h, [&](int& x) { n += x; }); } CHECK(n == 5 && p.live(b));
      static_assert(!has_first<cobpool<int, 8>> && has_first<cobcheckedhive<int, 8>>);
      static_assert(std::is_same_v<cobpool<int, 8>::handle, cobint<0, 8>>);
      cobcheckedpool<int, 8>::handle h; cobcheckedpool<int, 8> cp; CHECK(h == cp.nil);
      cobcheckedhive<int, 8> ch; auto x = ch.make(1); ch.make(2); ch.drop(x); CHECK(ch.next(x) == ch.nil);
      static_assert(sizeof(cobpool<char, 64>::handle) == sizeof(int)); }
    // coblist.md
    { coblist<int, 4> l; CHECK(l.insert(l.end(), 1) != l.end() && l.size() == 1);
      coblist<int, 4> x; x.push_back(5); x.push_back(6); auto xl = x.lbegin(); CHECK(l.splice(l.lend(), x) && x.empty() && !x.live(xl) && l.size() == 3);
      CHECK(!l.splice(l.lend(), l)); auto it = l.begin(); CHECK(--it == l.end()); it = l.end(); --it; CHECK(*it == 6);
      for (auto k = l.lbegin(); k != l.lend(); k = l.fwd(k)) { l.get(k); }
      cobfwdlist<int, 4> f; CHECK(f.insert_after(f.end(), 1) == f.end() && f.empty());
      static_assert(std::bidirectional_iterator<coblist<int, 4>::iterator> && std::forward_iterator<cobfwdlist<int, 4>::iterator>);
      static_assert(!std::bidirectional_iterator<cobfwdlist<int, 4>::iterator>);
      cobcheckedlist<int, 4> c; auto a = c.insert(c.lend(), 1); auto ci = c.begin(); c.erase(a); CHECK(c.fwd(a) == c.nil && c.bck(a) == c.nil && ++ci == c.end()); }
    // cobskiplist.md
    { cobtree<int, 8> t; CHECK(t.height() == 0 && t.sound()); for (int i = 0; i < 8; ++i) t.insert(i);
      CHECK(t.insert(9) == std::make_pair(t.end(), false) && !t.insert(3).second && *t.insert(3).first == 3);
      auto e = t.end(); --e; CHECK(*e == 7);
      static_assert(std::is_same_v<cobtree<int, 8>::iterator, cobtree<int, 8>::const_iterator>);
      static_assert(std::bidirectional_iterator<cobtree<int, 8>::iterator> && std::forward_iterator<cobskiplist<int, 8>::iterator> && !std::bidirectional_iterator<cobskiplist<int, 8>::iterator>);
      cobskiplist<int, 8> s; s.insert(1); std::ostringstream out; s.printme(out); CHECK(!out.str().empty()); }
    // cobmap.md
    { cobmap<std::string, int, 8> counts; std::string word = "a";
      for (int i = 0; i < 2; ++i) { if (!counts.with(word, [](int& n) { ++n; })) { counts.insert(word, 1); } }
      CHECK(counts.find<std::optional>("a") == 2);
      CHECK(counts.insert_or_assign("a", 5) == std::make_pair(counts.where("a"), false) && counts.emplace("b", 1).second);
      for (auto const& e : counts) { CHECK(!e.key.empty() && e.value > 0); }
      for (auto l = counts.lower_bound("a"); l != counts.nil; l = counts.fwd(l)) { counts.with(l, [](int& v) { ++v; }); }
      CHECK(counts.value<std::optional>(counts.where("b")) == 2 && counts.key<std::optional>(counts.lbegin()) == "a");
      cobmap<int, int, 1> one; one.insert(1, 1); CHECK(one.insert(2, 2) == std::make_pair(one.lend(), false)); }
    // cobflat.md
    { cobflatmap<std::string, int, 16> ports; ports.insert("http", 80); ports.with("http", [](int& p) { p = 8080; });
      CHECK(ports.find<std::optional>("http") == 8080 && ports.index_of<std::optional>("http")->get() == 0);
      static_assert(std::random_access_iterator<cobflatset<int, 4>::iterator>);
      cobflatset<int, 2> s; s.insert(1); s.insert(2); CHECK(s.insert(3) == std::make_pair(s.end(), false) && s.front<std::optional>() == 1 && s.back<std::optional>() == 2);
      CHECK(sizeof(cobflatset<int, 8>) == 9 * sizeof(int)); }
    // cobvector.md
    { cobvector<int, 4> v; v.push_back(3); v.push_back(1); std::sort(v.begin(), v.end()); for (auto i : v.irange()) { v.get(i); }
      CHECK(v.insert(cobic<2>, 9) && !v.insert(cobic<4>, 9) && v.swap(cobic<0>, cobic<2>) && !v.swap(cobic<0>, cobic<3>) && v.get(cobic<0>) == 9);
      auto it = v.begin(); it -= 1; CHECK(it == v.end()); CHECK(!v.resize(5) && v.resize(1) && v.capacity() == 4);
      cobvector<int, 4> w; CHECK((v.begin() - w.begin()) == 0 && !(v.begin() < w.begin()) && v.begin() != w.begin());
      static_assert(std::random_access_iterator<cobvector<int, 4>::iterator>); }
    // cobbinheap.md
    { cobbinheap<int, 16, std::greater<int>> h; h.push(3); h.push(1); CHECK(h.pop<std::optional>() == 1 && h.top<std::optional>() == 3 && h.with_top([](int const&) {})); }
    // cobhash.md, cobring.md, coblru.md
    { cobhash<std::string, int, 64> h; h.insert("one", 1); CHECK(h.find<std::optional>("one") == 1 && !h.insert("one", 2) && h.insert_or_assign("one", 2) && h.erase("one") && !h.erase("one"));
      cobring<int, 2> r; r.emplace_over(1); r.emplace_over(2); r.emplace_over(3); CHECK(r.at<std::optional>(0) == 2 && r.back<std::optional>() == 3 && !r.at<std::optional>(2));
      bool done = false; r.with_front([&](int&) { done = true; }); if (done) { r.pop_front<ignore>(); } CHECK(r.size() == 1);
      coblru<std::string, int, 2> c; int saved = 0; auto gone = [&](std::string const&, int& v) { saved += v; };
      c.put("a", 1, gone); c.put("b", 2, gone); c.put("a", 5, gone); CHECK(saved == 0); c.put("c", 3, gone); CHECK(saved == 2 && c.eldest<std::optional>() == "a");
      CHECK(c.peek<std::optional>("a") == 5 && c.eldest<std::optional>() == "a" && c.get<std::optional>("a") == 5 && c.eldest<std::optional>() == "c"); }
    // cobbitset.md
    { cobbitset<8> b; b.set(cobic<3>); CHECK(b.maybe_set(7) && !b.maybe_set(8)); int n = 0;
      for (auto i = b.first<std::optional>(); i; i = b.next<std::optional>(*i)) { n += i->get(); } CHECK(n == 10);
      static_assert(sizeof(cobbitset<64>) == 8 && sizeof(cobbitset<65>) == 16);
      static_assert(!std::is_convertible_v<int, cobbitset<8>::index>); }
    // cobstring.md
    { static_assert(sizeof(cobstring<15>) == 16);
      cobstring<15> s = "hello"; cobstring<31> u = s; CHECK(u == "hello" && u == s && s == std::string("hello") && s < "help" && std::string_view("a") < s);
      static_assert(!constructs<cobstring<4>, char const (&)[6]> && !constructs<cobstring<7>, cobstring<31> const&> && !constructs<cobstring<15>, std::string>);
      auto name = cobstring<15>::from<std::optional>(std::string(20, 'x')); CHECK(!name);
      CHECK(s.insert(0, s.view().substr(2)) && s == "llohello"); if (auto at = s.find<std::optional>('h')) { s.set(*at, 'j'); } CHECK(s == "llojello");
      CHECK(!s.find<std::optional>("").has_value() || s.find<std::optional>("")->get() == 0); CHECK(!s.find<std::optional>("", 8));
      CHECK(s.erase(3) && s == "llo" && !s.erase(4) && s.substr(5).empty() && s.get(cobic<9>) == '\0' && !s.set(cobic<9>, 'x'));
      std::ostringstream os; os << s; CHECK(os.str() == "llo" && std::hash<cobstring<15>>{}(s) == std::hash<std::string_view>{}("llo"));
      cobstring<3> lit = "a\0b"; CHECK(lit.size() == 1); }
    // cobarray.md, cobmatrix.md
    { cobarray<int, 3> a; CHECK(a.get(cobic<2>) == 0); cobarray<int, 3> b{ 1, 2, 3 }; CHECK(b.back() == 3 && a.set(cobic<0>, 4) == 4);
      static_assert(!constructs<cobarray<int, 3>, int> && sizeof(cobarray<int, 3>) == 4 * sizeof(int));
      cobarray<S, 3> as; as.set(cobic<0>, &S::f, 2.0f); float x = as.get(cobic<0>, &S::f); as.grab(cobic<0>)->f = 4.0; float y = as.grab(cobic<0>)->f; CHECK(x == 2.0f && y == 4.0f);
      for (auto i : a.irange()) { a.set(i, a.get(i) + 1); }
      auto squares = cobarray<int, 10>::generate([](cobint<0, 9> i) { return i.get() * i.get(); }); CHECK(squares.back() == 81);
      static_assert(std::random_access_iterator<cobarray<int, 3>::iterator>);
      cobmatrix<int, 3, 4> m; m.set({ cobic<2>, cobic<3> }, 7);
      for (auto r : cobirange<int, 0, 2>{}) { for (auto c : cobirange<int, 0, 3>{}) { m.set({ r, c }, 0); } }
      cobmatrix<cell, 8, 8> board; board.set({ cobic<0>, cobic<1> }, &cell::value, 5); board.grab({ cobic<0>, cobic<1> })->seen = true;
      using table = cobmatrix<int, 10, 10>; auto times = table::generate([](table::index i) { return (i.irow.get() + 1) * (i.icolumn.get() + 1); }); CHECK(times.back() == 100);
      static_assert(sizeof(cobmatrix<int, 3, 4>) == 13 * sizeof(int) && std::random_access_iterator<table::iterator>); }
    // atbdim.md
    { CHECK(atbdim<sessions>::set(5) && !atbdim<sessions>::set(6) && atbdim<points>::set(4));
      atblist<int, sessions> list; atbarray<int, sessions> stats; CHECK(list.max_size() == 5);
      if (auto i = atbidx<sessions>::be<std::optional>(2)) { stats.set(*i, {}); }
      for (auto i : atbidx<sessions>::range()) { stats.set(i, {}); }
      auto i = *atbidx<sessions>::first<std::optional>(); int n = 0; do { ++n; } while (i.advance()); CHECK(n == 5);
      atbarray<int, points> p; atbarray<int, edges> e; CHECK(e.size() == 12 && !atbdim<edges>::set(1));
      atbmatrix<int, points, points> am; CHECK(am.front() == 0); }
    // cobi.md
    { cobint<0, 5> x; cobint<0, 10> y = x; (void)y; static_assert(!std::is_convertible_v<cobint<0, 5>, cobint<0, 3>>);
      cobint<0, 5> port; using p05 = cobint<0, 5>; CHECK((!port.be(9) && port.be(5) && p05::could(0) && !p05::be<std::optional>(6)));
      CHECK((p05::clamp(99).get() == 5 && !port.advance())); port.advance_wrap(); CHECK(port.get() == 0);
      int n = 0; for (auto i : cobint<0, 5>::range()) { n += i; } for (auto i : cobirange<int, 0, 5>{}) { n += i.get(); } CHECK(n == 30);
      cobint<0, 2> a; cobint<2, 3> b; auto z = a + b; static_assert(std::is_same_v<decltype(z), cobint<2, 5>>); cobint<0, 5> u = a + b; (void)u;
      static_assert(!std::is_convertible_v<decltype(a + b), cobint<0, 4>>);
      constexpr bool same = (cobint<-5, -1>{} == cobint<0, 5>{}); static_assert(!same);
      static_assert(std::is_same_v<decltype(max(cobint<0, 9>{}, cobint<3, 4>{})), cobint<3, 9>> && std::is_same_v<decltype(abs(cobint<-5, 3>{})), cobint<0, 5>>);
      static_assert(cobint<0, 5>::DD == 0 && cobint<0, 5>::GG == 5 && sizeof(cobint<0, 5>) == sizeof(int));
      auto w = cobint<0, 5>{}.as<long long>() + cobi<long long, 1LL << 40>{}; (void)w; }
    // cobfunction.md
    { cobfunction<int(int)> f; auto r = f.call<std::optional>(5); CHECK(!r && f(5) == 0 && f.empty() && !f); f = [](int x) { return x + 1; }; r = f.call<std::optional>(5); CHECK(r == 6);
      cobfunction<void()> v; CHECK(!v.call()); v = [] {}; CHECK(v.call());
      int (*null)(int) = nullptr; CHECK(f.assign(null) && f.empty());
      char big[64] = {}; CHECK(!f.assign([big](int) { return int(big[0]); }) && f.capacity() == 3 * sizeof(void*));
      static_assert(!constructs<cobfunction<int(int)>, std::nullptr_t> && cobfunction<int(int)>::takes<int (*)(int)>);
      cobmovefunction<int()> m = [p = std::make_unique<int>(4)] { return *p; }; auto m2 = std::move(m); CHECK(m.empty() && m2() == 4);
      auto g = f; CHECK(g.empty()); CHECK(atbdim<bytes>::set(32)); atbfunction<int(int), bytes> af; CHECK(af.assign([](int x) { return x; }) && af(3) == 3);
      static_assert(!constructs<atbfunction<int(int), bytes>, int (*)(int)>);
      static_assert(sizeof(cobfunction<void()>) == 3 * sizeof(void*) + 2 * sizeof(void*) || sizeof(cobfunction<void()>) > 0); }
    // cobspsc.md
    { cobspsc<int, 64> q; auto tx = q.claim_producer<std::optional>(); auto rx = q.claim_consumer<std::optional>(); CHECK(!q.claim_producer<std::optional>());
      CHECK(!tx->full() && tx->push(1)); while (auto s = rx->pop<std::optional>()) { CHECK(*s == 1); }
      tx->push(2); bool sent = false; if (rx->with_front([&](int&) { sent = true; }) && sent) { rx->drop_front(); } CHECK(q.empty());
      CHECK(q.release(std::move(*tx))); auto tx2 = q.claim_producer<std::optional>(); CHECK(tx2.has_value() && !tx->push(1) && tx->full());
      static_assert(!std::is_default_constructible_v<cobspsc<int, 4>::producer> && !std::is_copy_constructible_v<cobspsc<int, 4>> && !std::is_move_constructible_v<cobspsc<int, 4>>);
      CHECK(sizeof(cobspsc<int, 64>) > 64 * sizeof(int)); }
    // perhaps.md, fullptr.md
    { perhaps<std::string> p(3u, 'a'); p.and_then([](std::string& s) { s += "!"; }); auto n = p.and_then([](std::string const& s) { return s.size(); }); CHECK(n == 4);
      perhaps<std::string> conv = std::string("x"); CHECK(conv.has_value() && perhaps<int>::unit(5).value_or(0) == 5);
      contingent<int*> cp; contingent<double> d(1.5); contingent<int, nothing_is<-1>> i(3); contingent<cobint<0, 5>> c; CHECK(!cp.has_value() && d.has_value() && i.has_value() && !c.has_value());
      CHECK(!(contingent<int, nothing_is<-1>>(-1).has_value()) && contingent<double>() == contingent<double>() && sizeof(contingent<double>) == sizeof(double));
      contingent<my_id> mine; CHECK(!mine.has_value() && contingent<my_id>(my_id{ 3 }).has_value());
      coblist<int*, 2> pl; auto b = pl.pop_front<contingently>(); CHECK(!b.has_value());
      int v = 5; nulifier<int, nothing_is<-1>> nul; CHECK(nul(v) && v == -1 && !nul(v));
      int x = 1; auto fp = fullptr<int>::from(x); auto q = make_fullptr(&x); q.and_then([](fullptr<int> k) { *k = 2; }); CHECK(*fp == 2 && !make_fullptr<std::optional>(static_cast<int*>(nullptr)));
      fullptr<int const> cfp = fp; CHECK(*cfp == 2); static_assert(!std::is_default_constructible_v<fullptr<int>> && sizeof(fullptr<int>) == sizeof(int*) && sizeof(optptr<int>) == sizeof(int*));
      static_assert(!std::is_default_constructible_v<optptr<int>> && !std::is_default_constructible_v<optshared<int>> && !std::is_default_constructible_v<optunique<int>> && !std::is_default_constructible_v<fullshared<int>>);
      optptr<int> next = &x; int depth = next.and_then([](int& k) { return k; }); CHECK(depth == 2 && next.release() == &x && !next);
      auto a = fullshared<std::string>::make("abc"); a->append("d"); optshared<std::string> o = a; o.and_then([](std::string& s) { s += "e"; }); CHECK(*a == "abcde");
      auto moved = std::move(a); CHECK(*a == "abcde" && moved == a); const optshared<std::string> co = o; co.and_then([](std::string& s) { s += "f"; });
      optshared<std::string> o2 = std::move(o); CHECK(!o && o2);
      auto buf = fullunique<std::string>::make(4u, 'z'); CHECK(buf->size() == 4);
      static_assert(!std::is_copy_constructible_v<fullunique<int>> && !std::is_move_constructible_v<fullunique<int>> && !std::is_copy_constructible_v<optunique<int>> && std::is_move_constructible_v<optunique<int>>);
      auto ou = optunique<int>::make(3); std::unique_ptr<int> up = ou.release(); CHECK(up && !ou); }
    // timers.md
    { cobtmrlist<int, 4, int> plain; plain.start(1, 5); int n = 0; plain.process_expired(5, [&](int) { ++n; }); CHECK(n == 1 && plain.empty());
      cobtmrmill<int, 512, ms, 3> mill; auto bad = mill.start(1, mill.max_duration + ms{ 1 }); auto good = mill.start(1, ms{ 5 }); CHECK(!bad.valid() && good.valid());
      n = 0; mill.process_expired(ms{ 5 }, [&](int) { ++n; }); CHECK(n == 0); mill.process_expired(ms{ 1 }, [&](int) { ++n; }); CHECK(n == 1);
      cobtmrmill<int, 512, ms, 3, 64, true> ex; ex.start(1, ms{ 5 }); n = 0; ex.process_expired(ms{ 5 }, [&](int) { ++n; }); CHECK(n == 0); ex.process_expired(ms{ 1 }, [&](int) { ++n; }); CHECK(n == 1);
      cobtmrwheel<int, 16, ms, 4> wh; wh.start(1, ms{ 5 }); wh.start(2, ms{ 0 }); n = 0; wh.process_expired(ms{ 1 }, [&](int id) { n += id; }); CHECK(n == 2); wh.process_expired(ms{ 4 }, [&](int id) { n += id; }); CHECK(n == 3);
      static_assert(std::is_same_v<cobcheckedtmrmill<int, 512, ms, 3, 64, true>, cobtmrmill<int, 512, ms, 3, 64, true>>);
      static_assert(std::is_same_v<cobtmrlist<int, 4, ms>::tmrID, int>);
      static_assert(cobtmrmill<int, 512, ms>::max_duration.count() == 1073741823);
      CHECK(wh.stop(wh.start(7, ms{ 3 })) == 0);
      cobtmrmill<int, 512, ms, 3, 64, true> full; int started = 0; for (int i = 0; i < 600; ++i) { started += full.start(i, ms{ 7 }).valid(); } CHECK(started == 512);
      cobtmrmill<int, 512, ms, 3> shared; started = 0; for (int i = 0; i < 600; ++i) { started += shared.start(i, ms{ 7 }).valid(); } CHECK(started == 512 / (3 * 64) + 1); }
    return 0;
}
