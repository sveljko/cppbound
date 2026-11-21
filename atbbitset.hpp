/* Allocation time bound bitset. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBBITSET)
#define      INC_ATBBITSET


#include "atbbound.hpp"
#include "cobbitset.hpp"


// A `cobbitset` whose capacity is the dimension of `Tag`, set at run time
template <class Tag>
using atbbitset = bndbitset<atbbound<Tag>>;


#endif // !defined(INC_ATBBITSET)
