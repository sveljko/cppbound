/* Allocation time bound hash table. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBHASH)
#define      INC_ATBHASH


#include "atbbound.hpp"
#include "cobhash.hpp"


// A `cobhash` whose capacity is the dimension of `Tag`, set at run time
template <class K, class V, class Tag, class H = std::hash<K>, class E = std::equal_to<K>>
using atbhash = bndhash<K, V, atbbound<Tag>, H, E>;


#endif // !defined(INC_ATBHASH)
