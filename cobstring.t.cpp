#include "cobstring.hpp"
#include "cobhash.hpp"
#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <cstring>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

using namespace std::string_view_literals;

// One character more than the capacity, and nothing else
static_assert(sizeof(cobstring<1>) == 2);
static_assert(sizeof(cobstring<5>) == 6);
static_assert(sizeof(cobstring<255>) == 256);

// All of it works at compile time
constexpr bool compiletime()
{
    cobstring<5> s;
    if (!s.empty() || (s.size() != 0) || (s.capacity() != 5) || (s.view().data()[0] != '\0')) {
        return false;
    }
    if (!s.append("ab") || (s.size() != 2) || (s.view().data()[2] != '\0')) {
        return false;
    }
    if (!s.append("cde") || !s.full() || (s.size() != 5) || (s.view().data()[5] != '\0')) {
        return false;
    }
    if (s.push_back('x') || !(s == "abcde")) {
        return false;
    }
    if (!s.erase(1, 2) || !(s == "ade") || !s.insert(1, "bc") || !(s == "abcde")) {
        return false;
    }
    if (!s.replace(1, 3, "X") || !(s == "aXe") || s.replace(0, 1, "123456") || !(s == "aXe")) {
        return false;
    }
    cobstring<5> lit = "hey";
    cobstring<9> wide = lit;
    return (lit.size() == 3) && (lit.get(cobic<1>) == 'e') && (lit.get(cobic<4>) == '\0') && (wide == lit)
           && (wide < "hi") && (lit.substr(1) == "ey");
}
static_assert(compiletime());
static_assert(cobstring<3>{ "abc" }.full());
static_assert(cobstring<8>{ "a\0b" }.size() == 1);    // a literal ends at its first NUL

// The "characters left" at the end, which is the NUL of a full string.
// There is no `c_str()`; a test may look behind a view.
void layout()
{
    cobstring<5> s;
    auto left = [&] { return static_cast<int>(static_cast<unsigned char>(s.view().data()[5])); };
    CHECK(left() == 5);
    CHECK(s.append("ab") && left() == 3 && std::strlen(s.view().data()) == 2);
    CHECK(s.append("cde") && left() == 0 && std::strlen(s.view().data()) == 5);
    CHECK(s.pop_back<std::optional>() == 'e' && left() == 1 && std::strlen(s.view().data()) == 4);
    s.clear();
    CHECK(left() == 5 && std::strlen(s.view().data()) == 0);

    cobstring<255> big;
    CHECK(big.assign(255, 'm') && big.full() && big.size() == 255 && !big.push_back('x'));
    CHECK(std::strlen(big.view().data()) == 255);
    CHECK(big.erase(0, 1) && big.size() == 254 && big.insert(254, 1, 'q') && big.get(cobic<254>) == 'q');
    CHECK(big.resize(0) && big.append(200, 'a') && !big.append(56, 'b') && big.append(55, 'b') && big.full());
}

void basics()
{
    cobstring<8> s = "abc";
    CHECK(s.size() == 3 && s.length() == 3 && s.capacity() == 8 && s.max_size() == 8 && !s.empty() && !s.full());
    CHECK(s.view() == "abc" && std::string_view(s) == "abc" && std::string(s.view()) == "abc");

    // by index: known to be within the capacity; past the size there is NUL
    CHECK(s.get(cobic<0>) == 'a' && s.get(cobic<2>) == 'c' && s.get(cobic<3>) == '\0' && s.get(cobic<7>) == '\0');
    CHECK(s.set(cobic<1>, 'B') && !s.set(cobic<3>, 'x') && s == "aBc");
    CHECK(s.front<std::optional>() == 'a' && s.back<perhaps>().value_or('?') == 'c');
    cobstring<8>::index i;
    CHECK(i.be(2) && s.get(i) == 'c' && !i.be(8));

    // what does not fit is not put in
    CHECK(s.append("defgh") && s.full() && s == "aBcdefgh");
    CHECK(!s.append("i") && !s.push_back('i') && !(s += "i") && !(s += 'i') && !s.insert(0, "i") && s == "aBcdefgh");
    CHECK(!s.assign("123456789") && !s.assign(9, 'x') && !s.resize(9) && !s.replace(0, 1, "12") && s == "aBcdefgh");
    CHECK(s.replace(0, 2, "12") && s == "12cdefgh");
    CHECK(s.assign("xy") && (s += "z") && (s += '!') && s == "xyz!");

    // positions past the end
    CHECK(s.insert(4, "?") && !s.insert(6, "?") && !s.erase(6) && !s.replace(6, 0, "") && s == "xyz!?");
    CHECK(s.erase(5) && s.erase(3, 100) && s == "xyz" && s.erase() && s.empty());
    CHECK(!s.front<std::optional>().has_value() && !s.back<std::optional>().has_value());
    CHECK(!s.pop_back<std::optional>().has_value());

    // a part of the string itself
    CHECK(s.assign("abc") && s.insert(0, s.view()) && s == "abcabc");
    CHECK(s.append(s.view().substr(4)) && s == "abcabcbc");
    CHECK(s.assign(s.view().substr(5)) && s == "cbc");
    CHECK(s.replace(1, 1, s) && s == "ccbcc");
    CHECK(s.insert(2, 2, '-') && s == "cc--bcc" && s.insert(7, 1, '.') && !s.insert(0, 1, '.'));

    CHECK(s.resize(3) && s == "cc-" && s.resize(5, '+') && s == "cc-++" && s.resize(6) && s.size() == 6);
    CHECK(s.get(cobic<5>) == '\0' && s.view() == "cc-++\0"sv);

    cobstring<8> t = "other";
    s.swap(t);
    CHECK(s == "other" && t.size() == 6);

    CHECK(s.substr() == "other" && s.substr(2) == "her" && s.substr(1, 2) == "th" && s.substr(5).empty()
          && s.substr(6).empty() && s.substr(3, 100) == "er");

    // from what may not fit
    CHECK(!cobstring<3>::from<std::optional>("abcd").has_value());
    CHECK(cobstring<3>::from<std::optional>(std::string("abc")).value() == "abc");
    CHECK(cobstring<3>::from<perhaps>("ab"sv).has_value());

    std::ostringstream os;
    os << s << '|' << cobstring<4>{};
    CHECK(os.str() == "other|");
}

void searching()
{
    cobstring<16> s = "hello, hello";
    CHECK(s.find<std::optional>("llo")->get() == 2 && s.find<std::optional>("llo", 3)->get() == 9);
    CHECK(!s.find<std::optional>("llo", 10).has_value() && !s.find<std::optional>("xyz").has_value());
    CHECK(s.find<std::optional>('o')->get() == 4 && !s.find<std::optional>('z').has_value());
    CHECK(s.rfind<std::optional>("hello")->get() == 7 && s.rfind<std::optional>("hello", 6)->get() == 0);
    CHECK(s.rfind<std::optional>('h')->get() == 7 && !s.rfind<std::optional>('z').has_value());
    CHECK(s.find_first_of<std::optional>(",o")->get() == 4 && s.find_last_of<std::optional>("h,")->get() == 7);
    CHECK(s.find_first_not_of<std::optional>("hel")->get() == 4 && s.find_last_not_of<std::optional>("lo")->get() == 8);
    CHECK(!s.find_first_not_of<std::optional>("helo, ").has_value());
    // what is found is an index: no check is needed to use it
    auto at = s.find<std::optional>(',');
    CHECK(at.has_value() && s.get(*at) == ',' && s.set(*at, ';') && s == "hello; hello");
    CHECK(s.find<perhaps>("lo").has_value() && !s.find<perhaps>("ol").has_value());
    // nothing is found where there is no character
    CHECK(s.find<std::optional>("")->get() == 0 && !s.find<std::optional>("", 12).has_value());
    CHECK(!s.rfind<std::optional>("").has_value() && s.rfind<std::optional>("", 11)->get() == 11);

    CHECK(s.contains("lo;") && !s.contains("lo,") && s.contains('h') && !s.contains('z'));
    CHECK(s.starts_with("hell") && !s.starts_with("ello") && s.ends_with("llo") && !s.ends_with("hell"));
    CHECK(s.compare("hello; hello") == 0 && s.compare("hello") > 0 && s.compare("help") < 0);
}

void comparing()
{
    cobstring<4>  a = "abc";
    cobstring<9>  b = "abc";
    cobstring<9>  c = "abd";
    cobstring<4>  e;
    CHECK(a == b && b == a && !(a != b) && a != c && c != a && a == a);
    CHECK(a < c && c > a && a <= b && a >= b && !(a < b) && e < a && !(a < e));
    CHECK(a == "abc" && "abc" == a && a != "abd" && a < "abd" && "abd" > a && a == std::string("abc"));
    CHECK(a == "abc"sv && a >= "ab"sv && a > "" && e == "");

    // a string that can't be longer converts; assigning it is the same
    cobstring<9> wide = a;
    CHECK(wide == "abc");
    wide = e;
    CHECK(wide.empty());
    wide = "xyz";
    CHECK(wide == "xyz");

    std::unordered_set<cobstring<9>> set;
    set.insert(b);
    set.insert(c);
    CHECK(set.count("abc") == 1 && set.count("abd") == 1 && set.count("abe") == 0 && set.size() == 2);
    cobhash<cobstring<9>, int, 8> h;
    CHECK(h.insert("one", 1) && h.insert("two", 2) && h.find<std::optional>("two") == 2);
}

void against_std()
{
    std::mt19937 rng(20241119);
    auto         word = [&] {
        std::string w;
        for (unsigned n = rng() % 6; n > 0; --n) {
            w += static_cast<char>('a' + rng() % 4);
        }
        return w;
    };
    for (int round = 0; round < 30; ++round) {
        cobstring<20> s;
        std::string   model;
        for (int step = 0; step < 2000; ++step) {
            const std::string w   = word();
            const unsigned    pos = rng() % 24;
            const unsigned    n   = rng() % 8;
            const unsigned    cut = (pos <= model.size()) ? static_cast<unsigned>(std::min<std::size_t>(n, model.size() - pos)) : 0;
            switch (rng() % 9) {
            case 0: {
                const bool ok = model.size() + w.size() <= 20;
                CHECK(s.append(w) == ok);
                if (ok) model.append(w);
                break;
            }
            case 1: {
                const bool ok = (pos <= model.size()) && (model.size() + w.size() <= 20);
                CHECK(s.insert(pos, w) == ok);
                if (ok) model.insert(pos, w);
                break;
            }
            case 2: {
                const bool ok = pos <= model.size();
                CHECK(s.erase(pos, n) == ok);
                if (ok) model.erase(pos, n);
                break;
            }
            case 3: {
                const bool ok = (pos <= model.size()) && (model.size() - cut + w.size() <= 20);
                CHECK(s.replace(pos, n, w) == ok);
                if (ok) model.replace(pos, n, w);
                break;
            }
            case 4: {
                const bool ok = pos <= 20;
                CHECK(s.resize(pos, '.') == ok);
                if (ok) model.resize(pos, '.');
                break;
            }
            case 5: {
                const auto c = s.pop_back<std::optional>();
                CHECK(c.has_value() == !model.empty());
                if (c) { CHECK(*c == model.back()); model.pop_back(); }
                break;
            }
            case 6: {
                const bool ok = (pos <= model.size()) && (model.size() + n <= 20);
                CHECK(s.insert(pos, n, '#') == ok);
                if (ok) model.insert(pos, n, '#');
                break;
            }
            case 7: {
                const auto f = s.find<std::optional>(w, pos);
                const auto r = s.rfind<std::optional>(w, pos);
                // "nowhere" is greater than any size; nothing found at the end is not found
                CHECK(f.has_value() == (model.find(w, pos) < model.size()));
                CHECK(r.has_value() == (model.rfind(w, pos) < model.size()));
                if (f) CHECK(static_cast<std::size_t>(f->get()) == model.find(w, pos));
                if (r) CHECK(static_cast<std::size_t>(r->get()) == model.rfind(w, pos));
                CHECK(s.substr(pos, n).view() == ((pos <= model.size()) ? model.substr(pos, n) : std::string()));
                break;
            }
            default: {
                const bool ok = s.assign(w);
                CHECK(ok);
                model = w;
            }
            }
            CHECK(s.view() == model && s.size() == model.size());
            CHECK(std::strlen(s.view().data()) == std::strlen(model.c_str()));
            CHECK(static_cast<unsigned char>(s.view().data()[20]) == 20 - model.size());
            CHECK((s.compare(w) < 0) == (model.compare(w) < 0) && (s == w) == (model == w));
        }
    }
}

int main()
{
    layout();
    basics();
    searching();
    comparing();
    against_std();
    return 0;
}
