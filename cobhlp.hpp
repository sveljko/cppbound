/* Compile time bound helpers. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBHLP)
#define INC_COBHLP

namespace cobhlp {
// TODO avoid defining this ourselves
    static constexpr unsigned log2(unsigned v)
    {
        unsigned r = (v > 0xFFFFU) << 4;
        v >>= r;
        unsigned shift = (v > 0xFFU) << 3;
        v >>= shift;
        r |= shift;
        shift = (v > 0xFU) << 2;
        v >>= shift;
        r |= shift;
        shift = (v > 0x3U) << 1;
        v >>= shift;
        r |= shift | (v >> 1);
        return r;
    }

    template <class T> struct less {
    constexpr bool operator(T const& l, T const& r) const {
        return l < r;
    )
};
  
};


#endif // !defined(INC_COBHLP)
