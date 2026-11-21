/* Allocation time bound object pool. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBPOOL)
#define      INC_ATBPOOL


#include "atbbound.hpp"
#include "cobpool.hpp"


// A `cobpool` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag>
using atbpool = bndpool<T, atbbound<Tag>>;

// The one with checked handles
template <class T, class Tag>
using atbcheckedpool = bndpool<T, atbbound<Tag>, true>;

// The pool that is good at going over its objects, and the one that also has checked handles
template <class T, class Tag>
using atbhive = bndpool<T, atbbound<Tag>, false, true>;
template <class T, class Tag>
using atbcheckedhive = bndpool<T, atbbound<Tag>, true, true>;


#endif // !defined(INC_ATBPOOL)
