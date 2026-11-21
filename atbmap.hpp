/* Allocation time bound map. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBMAP)
#define      INC_ATBMAP


#include "atbbound.hpp"
#include "cobmap.hpp"


// A `cobmap` whose capacity is the dimension of `Tag`, set at run time
template <class K, class V, class Tag, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32>
using atbmap = bndmap<K, V, atbbound<Tag>, CMP, RNG>;

// The one with checked links
template <class K, class V, class Tag, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32>
using atbcheckedmap = bndmap<K, V, atbbound<Tag>, CMP, RNG, true>;

// The ones where which slots hold an element is kept as a bit each (see `atbhive`)
template <class K, class V, class Tag, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32>
using atbhivemap = bndmap<K, V, atbbound<Tag>, CMP, RNG, false, true>;
template <class K, class V, class Tag, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32>
using atbcheckedhivemap = bndmap<K, V, atbbound<Tag>, CMP, RNG, true, true>;


#endif // !defined(INC_ATBMAP)
