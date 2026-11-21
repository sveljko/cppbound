/* Compile time bound binary heap. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBBINHEAP)
#define      INC_COBBINHEAP


#include "cobvector.hpp"

#include <utility>


// A priority queue: the element at the top is the one that no other is
// ordered after. So, like `std::priority_queue`, with "less" the greatest
// element is at the top; for the smallest one, order them by "greater".
template <class T, class B, class CMP = cobhlp::less<T>>
struct bndbinheap : private bndvector<T, B> {
    using vector     = bndvector<T, B>;
    using value_type = T;
    using size_type  = typename vector::size_type;

    using vector::clear;
    using vector::empty;
    using vector::full;
    using vector::max_size;
    using vector::size;

    // false if there is no room
    template <class... A> bool emplace(A&&... a)
    {
        if (!vector::emplace_back(std::forward<A>(a)...)) {
            return false;
        }
        rise(size() - 1);
        return true;
    }
    bool push(T const& t) { return emplace(t); }
    bool push(T&& t) { return emplace(std::move(t)); }

    // Takes the top out
    template <template <class> class V> V<T> pop()
    {
        if (empty()) {
            return {};
        }
        using std::swap;
        if (size() > 1) {
            swap(this->ref(0), this->ref(size() - 1));
        }
        V<T> rslt = this->template pop_back<V>();
        sink(0);
        return rslt;
    }
    // A copy of the top, or nothing
    template <template <class> class V> V<T> top() const { return this->template front<V>(); }
    // Calls f with the top, if there is one. Returns whether it did.
    template <class F> bool with_top(F&& f) const
    {
        if (empty()) {
            return false;
        }
        f(this->ref(0));
        return true;
    }
    // Calls f(T const&) for each element, in no particular order
    template <class F> void for_each(F&& f) const { vector::for_each(std::forward<F>(f)); }

private:
    bool before(size_type i, size_type j) const { return CMP{}(this->ref(i), this->ref(j)); }

    void rise(size_type i)
    {
        using std::swap;
        while (i > 0) {
            const size_type parent = (i - 1) / 2;
            if (!before(parent, i)) {
                break;
            }
            swap(this->ref(parent), this->ref(i));
            i = parent;
        }
    }
    void sink(size_type i)
    {
        using std::swap;
        const size_type n = size();
        for (;;) {
            size_type top = i;
            for (size_type child = 2 * i + 1; (child < n) && (child <= 2 * i + 2); ++child) {
                if (before(top, child)) {
                    top = child;
                }
            }
            if (top == i) {
                break;
            }
            swap(this->ref(i), this->ref(top));
            i = top;
        }
    }
};

template <class T, int N, class CMP = cobhlp::less<T>>
using cobbinheap = bndbinheap<T, cobbound<N>, CMP>;


#endif // !defined(INC_COBBINHEAP)
