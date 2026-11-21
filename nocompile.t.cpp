/* Things that must not compile. Each is compiled on its own, by defining
 * NOCOMPILE to its number. With NOCOMPILE not defined (or 0), this must
 * compile, to show that it's the offending line that fails, not the rest.
 */
#include "atbarray.hpp"
#include "atbstring.hpp"
#include "atbvector.hpp"
#include "cobarray.hpp"
#include "cobbinheap.hpp"
#include "cobbitset.hpp"
#include "cobflatmap.hpp"
#include "cobflatset.hpp"
#include "cobfunction.hpp"
#include "cobi.hpp"
#include "coblist.hpp"
#include "cobmap.hpp"
#include "cobmatrix.hpp"
#include "cobpool.hpp"
#include "cobskiplist.hpp"
#include "cobspsc.hpp"
#include "cobstring.hpp"
#include "cobtree.hpp"
#include "cobvector.hpp"
#include "contingent.hpp"
#include "fullunique.hpp"
#include "optptr.hpp"
#include "optshared.hpp"
#include "optunique.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <string>

#if !defined(NOCOMPILE)
#define NOCOMPILE 0
#endif

struct S {
    int i;
};

struct tag1 {};
struct tag2 {};

int main()
{
    constexpr auto imax = std::numeric_limits<int>::max();
    constexpr auto imin = std::numeric_limits<int>::lowest();
    constexpr auto umax = std::numeric_limits<unsigned>::max();

    cobint<0, 5>                 x;
    cobint<-3, 3>                mixed;
    cobi<unsigned, 0u, 5u>       u;
    cobarray<S, 5>               a;
    cobarray<S, 5> const&        ca = a;
    cobmatrix<int, 3, 2>         m;
    int                          i = 0;
    optptr<int>                  o(&i);
    perhaps<int>                 p{ 1 };
    cobskiplist<int, 8>          sl;
    contingent<int*>             cp;
    atbarray<int, tag1>          atb;
    cobbitset<10>                bits;
    cobvector<int, 4>            vec;
    cobstring<3>                 str;
    cobmap<int, int, 8>          mp;
    cobbinheap<int, 4>           heap;
    atbvector<int, tag1>         avec;
    cobtree<int, 8>              tree;
    cobpool<int, 4>              plain;
    cobcheckedpool<int, 4>       checked;
    cobspsc<int, 4>              queue;
    cobfunction<int(int), 8>     fn;
    cobmovefunction<int(int), 8> mfn;
    coblist<int, 4>              plainlist;
    cobcheckedlist<int, 4>       checkedlist;
    cobflatset<int, 8>           flatset;
    cobflatmap<int, int, 8>      flatmap;
    (void)mfn; (void)plainlist; (void)checkedlist; (void)flatset; (void)flatmap;
    (void)vec; (void)str; (void)mp; (void)heap; (void)avec;
    (void)tree; (void)plain; (void)checked; (void)queue; (void)fn;
    (void)bits;
    (void)atb;
    optshared<S>                 os(nullptr);
    optunique<S>                 ou(nullptr);
    auto                         fu = fullunique<int>::make(1);
    (void)os; (void)ou; (void)fu;
    (void)cp;
    (void)sl;
    (void)imax; (void)imin; (void)umax; (void)x; (void)mixed; (void)u; (void)ca; (void)m; (void)o; (void)p;

#if NOCOMPILE == 1 // a value from a wider range
    cobint<0, 5> z = x + cobint<0, 1>{};
#elif NOCOMPILE == 2 // an integer that was not checked
    cobint<0, 5> z = 3;
#elif NOCOMPILE == 3 // assigning a value from a wider range
    x = cobint<0, 6>{};
#elif NOCOMPILE == 4 // addition overflow
    auto z = cobint<imax - 1>{} + cobic<2>;
#elif NOCOMPILE == 5 // subtraction overflow
    auto z = cobint<imin + 1>{} - cobic<2>;
#elif NOCOMPILE == 6 // multiplication overflow
    auto z = cobint<0, imax / 2 + 1>{} * cobic<2>;
#elif NOCOMPILE == 7 // negation overflow
    auto z = -cobint<imin, 0>{};
#elif NOCOMPILE == 8 // division overflow
    auto z = cobint<imin, 0>{} / cobint<-2, -1>{};
#elif NOCOMPILE == 9 // possible division by zero
    auto z = x / mixed;
#elif NOCOMPILE == 10 // division by a range that starts at zero
    auto z = x / x;
#elif NOCOMPILE == 11 // possible modulo by zero
    auto z = x % mixed;
#elif NOCOMPILE == 12 // unsigned addition overflow (wraps)
    auto z = cobi<unsigned, 0u, umax>{} + cobi<unsigned, 0u, 1u>{};
#elif NOCOMPILE == 13 // unsigned subtraction underflow (wraps)
    auto z = u - cobi<unsigned, 0u, 1u>{};
#elif NOCOMPILE == 14 // unsigned multiplication overflow (wraps)
    auto z = cobi<unsigned, 0u, umax / 2 + 1>{} * cobi<unsigned, 2u>{};
#elif NOCOMPILE == 15 // negative unsigned
    auto z = -u;
#elif NOCOMPILE == 16 // range needs a value past the upper bound
    auto z = cobint<0, imax>::range();
#elif NOCOMPILE == 17 // array index out of bounds
    auto z = a.get(cobic<5>);
#elif NOCOMPILE == 18 // array index from a wider range
    a.set(cobint<0, 5>{}, S{});
#elif NOCOMPILE == 19 // array index that was not checked
    auto z = a.get(3);
#elif NOCOMPILE == 20 // matrix row out of bounds
    auto z = m.get({ cobic<3>, cobic<0> });
#elif NOCOMPILE == 21 // matrix column out of bounds
    m.set({ cobic<0>, cobic<2> }, 1);
#elif NOCOMPILE == 22 // keeping what was grabbed
    auto t = a.grab(cobic<0>);
    t->i   = 1;
#elif NOCOMPILE == 23 // changing a constant array
    ca.grab(cobic<0>)->i = 1;
#elif NOCOMPILE == 24 // fullptr from a pointer that was not checked
    fullptr<int> f(&i);
#elif NOCOMPILE == 25 // fullptr to nothing
    fullptr<int> f;
#elif NOCOMPILE == 26 // unchecked access to an optptr
    auto z = *o;
#elif NOCOMPILE == 27 // unchecked access to a perhaps
    auto z = *p;
#elif NOCOMPILE == 28 // optptr is not a number
    int z = o;
#elif NOCOMPILE == 29 // changing an element of a skip list
    *sl.begin() = 1;
#elif NOCOMPILE == 30 // a range that does not fit the other type
    auto z = cobint<0, 300>{}.as<unsigned char>();
#elif NOCOMPILE == 31 // absolute value of the lowest integer overflows
    auto z = abs(cobint<imin, 0>{});
#elif NOCOMPILE == 32 // too few values
    cobarray<int, 3> z{ 1, 2 };
#elif NOCOMPILE == 33 // too many values
    cobarray<int, 3> z{ 1, 2, 3, 4 };
#elif NOCOMPILE == 34 // fullptr to an unrelated type
    long l = 0;
    fullptr<int> f = fullptr<long>::from(l);
#elif NOCOMPILE == 35 // unchecked access to a contingent
    auto z = *cp;
#elif NOCOMPILE == 36 // no integer left to mean nothing
    contingent<cobi<int, imin, imax>> z;
#elif NOCOMPILE == 37 // no policy for this type
    contingent<std::string> z;
#elif NOCOMPILE == 38 // dereferencing an optshared
    auto z = *os;
#elif NOCOMPILE == 39 // member access through an optshared
    auto z = os->i;
#elif NOCOMPILE == 40 // dereferencing an optunique
    auto z = *ou;
#elif NOCOMPILE == 41 // a fullshared that points to nothing
    fullshared<int> f;
#elif NOCOMPILE == 42 // moving a fullunique
    fullunique<int> b = std::move(fu);
#elif NOCOMPILE == 43 // copying a fullunique
    fullunique<int> b = fu;
#elif NOCOMPILE == 44 // an atbarray index that was not checked
    auto z = atb.get(3);
#elif NOCOMPILE == 45 // a compile time bound integer is not an atbarray index
    auto z = atb.get(cobic<0>);
#elif NOCOMPILE == 46 // an index of one tag on an array of another
    auto j = *atbidx<tag2>::first<std::optional>();
    auto z = atb.get(j);
#elif NOCOMPILE == 47 // there is no index out of thin air
    atbidx<tag1> j;
#elif NOCOMPILE == 48 // a bit out of range
    auto z = bits.test(cobic<10>);
#elif NOCOMPILE == 49 // a bit that was not checked
    bits.set(3);
#elif NOCOMPILE == 50 // vector index out of the capacity
    auto z = vec.get(cobic<4>);
#elif NOCOMPILE == 51 // vector index that was not checked
    auto z = vec.get(1);
#elif NOCOMPILE == 52 // a literal that does not fit the string
    cobstring<3> z = "abcd";
#elif NOCOMPILE == 53 // character index out of the capacity
    auto z = str.get(cobic<3>);
#elif NOCOMPILE == 54 // a string of more than 255 characters
    cobstring<256> z;
#elif NOCOMPILE == 55 // a string that may be too long
    cobstring<3> z = cobstring<4>{};
#elif NOCOMPILE == 56 // changing a key of a map
    mp.for_each([](int& k, int&) { k = 1; });
#elif NOCOMPILE == 57 // changing a value of a map through an iterator
    mp.begin()->value = 1;
#elif NOCOMPILE == 58 // changing an element of a heap
    heap.for_each([](int& x) { x = 0; });
#elif NOCOMPILE == 59 // no literal is known to fit a string bound at allocation time
    atbstring<tag1> z = "ab";
#elif NOCOMPILE == 60 // a compile time bound integer is not an atbvector index
    auto z = avec.get(cobic<0>);
#elif NOCOMPILE == 61 // an index of one tag on a vector of another
    auto j = *atbidx<tag2>::first<std::optional>();
    auto z = avec.get(j);
#elif NOCOMPILE == 62 // a reference to an element of a vector
    int& z = vec.get(cobic<0>);
#elif NOCOMPILE == 63 // a string has no iterators
    auto z = str.begin();
#elif NOCOMPILE == 64 // nor a pointer to its characters
    auto z = str.c_str();
#elif NOCOMPILE == 65 // a constant iterator of a vector does not become one that is not
    cobvector<int, 4>::I z = vec.cbegin();
#elif NOCOMPILE == 66 // a dimension that is worked out can't be made up
    atbidx<atbpart<tag1, 2>> z{ 0 };
#elif NOCOMPILE == 67 // changing an element of a tree
    *tree.begin() = 1;
#elif NOCOMPILE == 68 // a handle that is not checked is not one that is
    auto z = checked.live(plain.make(1));
#elif NOCOMPILE == 69 // a checked handle can't be made up
    cobcheckedpool<int, 4>::handle z{ cobic<1> };
#elif NOCOMPILE == 70 // a queue between two threads is not copied
    auto z = queue;
#elif NOCOMPILE == 71 // what does not fit the function
    struct { char pad[9]; int operator()(int x) const { return x; } } wide;
    fn = wide;
#elif NOCOMPILE == 72 // what can't be called as the function is
    cobfunction<int(int), 8> z = [](int*) { return 1; };
#elif NOCOMPILE == 73 // what can't be copied can't be held by a function
    cobfunction<void()> z = [p = std::unique_ptr<int>()] {};
#elif NOCOMPILE == 74 // putting in a queue is for its producer
    queue.push(1);
#elif NOCOMPILE == 75 // the producer of a queue is not copied
    auto tx = queue.claim_producer<std::optional>();
    auto z  = *tx;
#elif NOCOMPILE == 76 // nor made up
    cobspsc<int, 4>::consumer z;
#elif NOCOMPILE == 77 // a function that can only be moved is not copied
    auto z = mfn;
#elif NOCOMPILE == 78 // a link that is not checked is not one that is
    checkedlist.erase(plainlist.lbegin());
#elif NOCOMPILE == 79 // a checked link is not a number
    auto z = checkedlist.get(cobic<0>);
#elif NOCOMPILE == 80 // changing an element of a flat set
    *flatset.begin() = 1;
#elif NOCOMPILE == 81 // changing a key of a flat map
    flatmap.begin()->key = 1;
#elif NOCOMPILE == 82 // nor its value, through an iterator
    flatmap.begin()->value = 1;
#elif NOCOMPILE == 83 // flat set index out of the capacity
    auto z = flatset.get(cobic<8>);
#elif NOCOMPILE == 84 // a pool that is not a hive is not walked
    auto z = plain.first();
#elif NOCOMPILE == 85 // a side of a queue is given back, not copied back
    auto tx = queue.claim_producer<std::optional>();
    queue.release(*tx);
#endif

    return 0;
}
