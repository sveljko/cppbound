/* Allocation time bound queue between two threads. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBSPSC)
#define      INC_ATBSPSC


#include "atbbound.hpp"
#include "cobspsc.hpp"


// A `cobspsc` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag>
using atbspsc = bndspsc<T, atbbound<Tag>>;


#endif // !defined(INC_ATBSPSC)
