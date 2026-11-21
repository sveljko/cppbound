/* Compile time bound string. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBSTRING)
#define      INC_COBSTRING


#include "cobbound.hpp"

#include <compare>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string_view>
#include <utility>


// A string of at most as many characters as its bound says, and never more
// than 255. There is one more character than that: the last one holds the
// number of characters that can still be added. When the string is full,
// that is 0, which is also what terminates the string then.
//
// Like `std::string`, less what is unsafe: there is no reference or
// pointer to a character and no iterators, what does not fit is not put in (`false` is
// returned and the string stays as it was) and what is not there is not
// given (see `get()` and the "generic optional" results).
template <class B>
struct bndstring {
    using value_type = char;
    using size_type  = unsigned;
    using index      = typename B::index;   // of a character: known to be within the capacity

    // For the count of characters to take: as many as there are
    static constexpr size_type npos = ~size_type{ 0 };

    constexpr bndstring() { length_is(0); }

    // A string literal that fits: checked at compile time
    template <std::size_t M>
        requires(B::fixed && (M - 1 <= B::size()))
    constexpr bndstring(char const (&s)[M]) : bndstring()
    {
        size_type n = 0;
        while ((n < M - 1) && (s[n] != '\0')) {
            at(n) = s[n];
            ++n;
        }
        length_is(n);
    }
    // A string that can't be longer than this one
    template <class B2>
        requires(B::fixed && B2::fixed && (B2::size() <= B::size()))
    constexpr bndstring(bndstring<B2> const& x) : bndstring() { assign(x.view()); }

    // Nothing, if `x` does not fit
    template <template <class> class V> static constexpr V<bndstring> from(std::string_view x)
    {
        bndstring s;
        return s.assign(x) ? V<bndstring>(s) : V<bndstring>{};
    }

    constexpr size_type size() const noexcept { return capacity() - left(); }
    constexpr size_type length() const noexcept { return size(); }
    constexpr size_type capacity() const noexcept { return (B::size() < 255) ? static_cast<size_type>(B::size()) : 255; }
    constexpr size_type max_size() const noexcept { return capacity(); }
    constexpr bool      empty() const noexcept { return size() == 0; }
    constexpr bool      full() const noexcept { return left() == 0; }

    // The way to pass the string on. A view is meant to be a parameter, not
    // to be kept: it refers to the characters of the string, so it is of no
    // use once the string is gone or changed. That is also why there is no
    // `c_str()`: a pointer to characters is what gets kept.
    constexpr std::string_view view() const noexcept { return { d.data(), size() }; }
    constexpr operator std::string_view() const noexcept { return view(); }

    // The character at `i`; past the last one, that is the terminating NUL
    constexpr char get(index i) const { return live(i) ? at(pos(i)) : '\0'; }
    // false if there is no character at `i`
    constexpr bool set(index i, char c)
    {
        if (!live(i)) {
            return false;
        }
        at(pos(i)) = c;
        return true;
    }
    template <template <class> class V> constexpr V<char> front() const
    {
        return empty() ? V<char>{} : V<char>(at(0));
    }
    template <template <class> class V> constexpr V<char> back() const
    {
        return empty() ? V<char>{} : V<char>(at(size() - 1));
    }

    // What follows changes the string and returns `false`, with the string
    // as it was, if the result would not fit or the position is past the end

    constexpr bool assign(std::string_view x)
    {
        if (x.size() > capacity()) {
            return false;
        }
        const size_type n = static_cast<size_type>(x.size());
        for (size_type i = 0; i < n; ++i) {     // forward: `x` may be a part of this string
            at(i) = x[i];
        }
        length_is(n);
        return true;
    }
    constexpr bool assign(size_type n, char c)
    {
        if (n > capacity()) {
            return false;
        }
        length_is(0);
        return append(n, c);
    }

    constexpr bool push_back(char c) { return append(1, c); }
    constexpr bool append(size_type n, char c)
    {
        if (n > left()) {
            return false;
        }
        const size_type len = size();
        for (size_type i = 0; i < n; ++i) {
            at(len + i) = c;
        }
        length_is(len + n);
        return true;
    }
    constexpr bool append(std::string_view x) { return replace(size(), 0, x); }
    constexpr bool operator+=(std::string_view x) { return append(x); }
    constexpr bool operator+=(char c) { return push_back(c); }

    template <template <class> class V> constexpr V<char> pop_back()
    {
        const size_type len = size();
        if (len == 0) {
            return {};
        }
        V<char> rslt(at(len - 1));
        length_is(len - 1);
        return rslt;
    }

    constexpr bool insert(size_type at_, std::string_view x) { return replace(at_, 0, x); }
    constexpr bool insert(size_type at_, size_type n, char c)
    {
        const size_type len = size();
        if ((at_ > len) || (n > left())) {
            return false;
        }
        for (size_type i = len; i > at_; --i) {
            at(i - 1 + n) = at(i - 1);
        }
        for (size_type i = 0; i < n; ++i) {
            at(at_ + i) = c;
        }
        length_is(len + n);
        return true;
    }
    // `n` characters from `at_`, or as many as there are
    constexpr bool erase(size_type at_ = 0, size_type n = npos) { return replace(at_, n, {}); }
    // `n` characters from `at_`, or as many as there are, by `x`
    constexpr bool replace(size_type at_, size_type n, std::string_view x)
    {
        const size_type len = size();
        if (at_ > len) {
            return false;
        }
        if (n > len - at_) {
            n = len - at_;
        }
        if (x.size() > capacity() - (len - n)) {
            return false;
        }
        // `x` may be a part of this string: copy it before anything is moved
        char            fresh[255] = {};
        const size_type m          = static_cast<size_type>(x.size());
        for (size_type i = 0; i < m; ++i) {
            fresh[i] = x[i];
        }
        if (m > n) {    // the rest goes up, from its end
            for (size_type i = len; i > at_ + n; --i) {
                at(i - 1 + (m - n)) = at(i - 1);
            }
        }
        else if (m < n) {   // the rest goes down, from its start
            for (size_type i = at_ + n; i < len; ++i) {
                at(i - (n - m)) = at(i);
            }
        }
        for (size_type i = 0; i < m; ++i) {
            at(at_ + i) = fresh[i];
        }
        length_is(len - n + m);
        return true;
    }
    // Cuts the end off, or adds `c` to it
    constexpr bool resize(size_type n, char c = '\0')
    {
        const size_type len = size();
        if (n > capacity()) {
            return false;
        }
        if (n > len) {
            return append(n - len, c);
        }
        length_is(n);
        return true;
    }
    constexpr void clear() { length_is(0); }
    constexpr void swap(bndstring& x)
    {
        bndstring t = *this;
        *this       = x;
        x           = t;
    }

    // `n` characters from `at_`, or as many as there are; none, if `at_` is past the end
    constexpr bndstring substr(size_type at_ = 0, size_type n = npos) const
    {
        bndstring r;
        if (at_ <= size()) {
            r.assign(view().substr(at_, n));
        }
        return r;
    }

    // The index of the character where it was found, or nothing
    template <template <class> class V> constexpr V<index> find(std::string_view x, size_type from = 0) const
    {
        return found<V>(view().find(x, from));
    }
    template <template <class> class V> constexpr V<index> find(char c, size_type from = 0) const
    {
        return found<V>(view().find(c, from));
    }
    template <template <class> class V> constexpr V<index> rfind(std::string_view x, size_type from = npos) const
    {
        return found<V>(view().rfind(x, from));
    }
    template <template <class> class V> constexpr V<index> rfind(char c, size_type from = npos) const
    {
        return found<V>(view().rfind(c, from));
    }
    template <template <class> class V>
    constexpr V<index> find_first_of(std::string_view x, size_type from = 0) const
    {
        return found<V>(view().find_first_of(x, from));
    }
    template <template <class> class V>
    constexpr V<index> find_last_of(std::string_view x, size_type from = npos) const
    {
        return found<V>(view().find_last_of(x, from));
    }
    template <template <class> class V>
    constexpr V<index> find_first_not_of(std::string_view x, size_type from = 0) const
    {
        return found<V>(view().find_first_not_of(x, from));
    }
    template <template <class> class V>
    constexpr V<index> find_last_not_of(std::string_view x, size_type from = npos) const
    {
        return found<V>(view().find_last_not_of(x, from));
    }
    constexpr bool contains(std::string_view x) const { return view().find(x) != std::string_view::npos; }
    constexpr bool contains(char c) const { return view().find(c) != std::string_view::npos; }
    constexpr bool starts_with(std::string_view x) const { return view().starts_with(x); }
    constexpr bool ends_with(std::string_view x) const { return view().ends_with(x); }
    constexpr int  compare(std::string_view x) const { return view().compare(x); }

    constexpr bool operator==(std::string_view x) const { return view() == x; }
    constexpr auto operator<=>(std::string_view x) const { return view() <=> x; }
    template <class B2> constexpr bool operator==(bndstring<B2> const& x) const { return view() == x.view(); }
    template <class B2> constexpr auto operator<=>(bndstring<B2> const& x) const { return view() <=> x.view(); }

private:
    // All access to the characters is here; `i` is at most the capacity
    constexpr char&       at(size_type i) { return d.data()[i]; }
    constexpr char const& at(size_type i) const { return d.data()[i]; }

    // How many characters can still be added
    constexpr size_type left() const noexcept
    {
        return static_cast<unsigned char>(at(capacity()));
    }
    // `n` is at most the capacity
    constexpr void length_is(size_type n)
    {
        const size_type cap = capacity();
        at(cap)             = static_cast<char>(static_cast<unsigned char>(cap - n));
        at(n)               = (n < cap) ? '\0' : at(cap);
    }

    static constexpr size_type pos(index i) { return static_cast<size_type>(i.get()); }
    constexpr bool live(index i) const { return static_cast<std::size_t>(i.get()) < size(); }

    // What was found is given as the index of a character: nothing was
    // found where there is none (nothing at all, at the end of the string)
    template <template <class> class V> constexpr V<index> found(std::size_t at_) const
    {
        if (at_ >= size()) {
            return {};
        }
        return index::template be<V>(static_cast<decltype(std::declval<index>().get())>(at_));
    }

    typename B::template buffer<char> d;
};

template <int N>
    requires((N > 0) && (N <= 255))
using cobstring = bndstring<cobbound<N>>;

template <class B> std::ostream& operator<<(std::ostream& os, bndstring<B> const& s)
{
    return os << s.view();
}

template <class B> struct std::hash<bndstring<B>> {
    std::size_t operator()(bndstring<B> const& s) const noexcept
    {
        return std::hash<std::string_view>{}(s.view());
    }
};


#endif // !defined(INC_COBSTRING)
