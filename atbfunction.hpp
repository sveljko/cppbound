/* Allocation time bound callable. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBFUNCTION)
#define      INC_ATBFUNCTION


#include "atbbound.hpp"
#include "cobfunction.hpp"


// A `cobfunction` with as many bytes of room as the dimension of `Tag`, set at
// run time; the room is allocated when it is made. As it is not known at
// compile time what fits, there is no making it from what is to be called:
// use `assign()`, which tells if that did fit.
template <class S, class Tag>
using atbfunction = bndfunction<S, atbbound<Tag>>;

// The one that can only be moved, and so holds also what can only be moved
template <class S, class Tag>
using atbmovefunction = bndfunction<S, atbbound<Tag>, false>;


#endif // !defined(INC_ATBFUNCTION)
