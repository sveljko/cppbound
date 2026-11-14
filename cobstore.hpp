/* Raw storage for N objects, for the containers to build on.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBSTORE)
#define      INC_COBSTORE


#include "cobi.hpp"

#include <new>
#include <utility>


// The store does not know which slots hold an object: its owner must only
// call `at`/`drop` on a slot it made, and `make` on a slot that is empty.
template <class T, int N> class cobstore {
    static_assert(N > 0, "Store must have some capacity");
    union slot {
        char none;
        T    some;
        constexpr slot() : none() {}
        ~slot() {}
    };
    slot s[N];

public:
    using index = cobint<0, N - 1>;

    cobstore() = default;
    cobstore(cobstore const&) = delete;            // the owner knows which slots are live
    cobstore& operator=(cobstore const&) = delete;

    template <class... A> void make(index i, A&&... a) {
        ::new (static_cast<void*>(&s[i.get()].some)) T(std::forward<A>(a)...);
    }
    void     drop(index i) { s[i.get()].some.~T(); }
    T&       at(index i) { return s[i.get()].some; }
    T const& at(index i) const { return s[i.get()].some; }
};


#endif // !defined(INC_COBSTORE)
