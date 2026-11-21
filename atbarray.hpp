/* Allocation time bound array. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBARRAY)
#define      INC_ATBARRAY


#include "atbbound.hpp"


// An array of as many elements as the dimension of `Tag`, set at run time,
// indexed by an `atbidx<Tag>`
template <class T, class Tag> using atbarray = bndarray<T, atbbound<Tag>>;


#endif // !defined(INC_ATBARRAY)
