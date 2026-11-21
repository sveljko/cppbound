/* Allocation time bound vector. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBVECTOR)
#define      INC_ATBVECTOR


#include "atbbound.hpp"
#include "cobvector.hpp"


// A `cobvector` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag>
using atbvector = bndvector<T, atbbound<Tag>>;


#endif // !defined(INC_ATBVECTOR)
