/* Compile time bound arrays. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBARRAY)
#define      INC_COBARRAY


#include "cobbound.hpp"


// An array of N elements, indexed by a `cobint<0, N-1>`
template <class T, int N> using cobarray = bndarray<T, cobbound<N>>;


#endif // define      INC_COBARRAY
