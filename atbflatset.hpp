/* Allocation time bound flat set. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBFLATSET)
#define      INC_ATBFLATSET


#include "atbbound.hpp"
#include "cobflatset.hpp"


// A `cobflatset` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag, class CMP = cobhlp::less<T>>
using atbflatset = bndflatset<T, atbbound<Tag>, CMP>;


#endif // !defined(INC_ATBFLATSET)
