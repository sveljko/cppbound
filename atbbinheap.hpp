/* Allocation time bound binary heap. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBBINHEAP)
#define      INC_ATBBINHEAP


#include "atbbound.hpp"
#include "cobbinheap.hpp"


// A `cobbinheap` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag, class CMP = cobhlp::less<T>>
using atbbinheap = bndbinheap<T, atbbound<Tag>, CMP>;


#endif // !defined(INC_ATBBINHEAP)
