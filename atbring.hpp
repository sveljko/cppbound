/* Allocation time bound ring buffer. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBRING)
#define      INC_ATBRING


#include "atbbound.hpp"
#include "cobring.hpp"


// A `cobring` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag>
using atbring = bndring<T, atbbound<Tag>>;


#endif // !defined(INC_ATBRING)
