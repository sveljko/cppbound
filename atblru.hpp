/* Allocation time bound least-recently-used cache. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBLRU)
#define      INC_ATBLRU


#include "atbbound.hpp"
#include "coblru.hpp"


// A `coblru` whose capacity is the dimension of `Tag`, set at run time
template <class K, class V, class Tag, class H = std::hash<K>, class E = std::equal_to<K>>
using atblru = bndlru<K, V, atbbound<Tag>, H, E>;

// The one where which slots hold an entry is kept as a bit each (see `atbhive`)
template <class K, class V, class Tag, class H = std::hash<K>, class E = std::equal_to<K>>
using atbhivelru = bndlru<K, V, atbbound<Tag>, H, E, true>;


#endif // !defined(INC_ATBLRU)
