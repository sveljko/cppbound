#if !defined(INC_COBTESTHELP)
#define INC_COBTESTHELP


#include <cstdlib>
#include <iostream>

#include "cobarray.hpp"

// Unlike `assert()`, this is checked in all build types
#define CHECK(x)                                                              \
    do {                                                                      \
        if (!(x)) {                                                           \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: "    \
                      << #x << std::endl;                                     \
            std::abort();                                                     \
        }                                                                     \
    } while (0)

// Counts the objects alive, and how they came about
struct counted {
    static inline int alive  = 0;
    static inline int copies = 0;
    static inline int moves  = 0;
    int               v;

    counted(int v_ = 0) : v(v_) { ++alive; }
    counted(counted const& x) : v(x.v) { ++alive; ++copies; }
    counted(counted&& x) noexcept : v(x.v) { ++alive; ++moves; x.v = -1; }
    ~counted() { --alive; }
};

struct nodefault {          // no default constructor, not assignable from int
    explicit nodefault(int x) : v(x) {}
    int v;
};

template <int N, int D, int G>
void print(std::ostream& out, cobarray<cobint<D, G>, N> const &l)
{
    for (auto &x : l)
    {
        out << x.get() << " ";
    }
    out << "." << std::endl;
}

template <int N, int D, int G>
void print(cobarray<cobint<D, G>, N> const &l) {
    print(std::cout, l);
}


#endif // !defined(INC_COBTESTHELP)
