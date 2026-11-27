/* Compile time bound timer lean ring. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRLEANRING)
#define      INC_COBTMRLEANRING


#include "cobbound.hpp"

#include <utility>


// Timers kept sorted by when they expire, in a ring: no links, no pool, just
// the IDs, their expiry times, and the head and tail of the ring. For where
// memory is tight and timers mostly expire: cancelling only nullifies the ID
// in place (as in the lean mill), and the hole it leaves is used by a later
// start, either directly, if the new timer fits there, or by the shift that
// inserting makes, which stops at the hole.
// There is no index of a timer, as inserting moves timers around.
template <class ID, class B, class U, class NULIFY> class bndtmrleanring {
    struct elem {
        ID id;
        U  due;     // when it expires, on the `now` clock
    };
    using size_type = typename B::size_type;
    using slot      = typename B::slot;

    typename B::template buffer<elem> d;
    slot      head, tail;       // the ring is [head, tail): live timers and holes
    size_type count = 0;        // how many of them are live
    U         now{};

public:
    using tmrID = ID;

    bndtmrleanring() = default;

    constexpr bool empty() const { return count == 0; }

    // false if there is no room, or the duration is negative
    bool start(ID id, U duration)
    {
        if ((duration < U{ 0 }) || (count == B::size())) {
            return false;
        }
        trim();
        const U         due = now + duration;
        const size_type n = B::size();
        const size_type l = arc();
        // Where the new timer goes: `p`, before the first live timer that
        // expires no sooner than it; `h`, a hole it fits in, between the same
        // live neighbours; `h3`, the last hole before `p`
        size_type p = l, h = n, h3 = n;
        for (size_type i = 0; i < l; ++i) {
            elem const& e = at(i);
            if (live(e.id)) {
                if (due <= e.due) {
                    p = i;
                    break;
                }
                h = n;      // a hole before this one can not hold the new timer
            }
            else {
                h3 = i;
                if (h == n) {
                    h = i;
                }
            }
        }
        if (h != n) {       // it fits in a hole: nothing moves
            at(h) = elem{ std::move(id), due };
            ++count;
            return true;
        }
        if ((p == 0) && (l < n)) {      // room before the head: just step back
            back(head);
            at(0) = elem{ std::move(id), due };
            ++count;
            return true;
        }
        size_type h2 = n;   // the first hole the shift can stop at
        for (size_type i = p; i < l; ++i) {
            if (!live(at(i).id)) {
                h2 = i;
                break;
            }
        }
        if (h2 != n) {      // shift toward the tail, into the hole
            for (size_type i = h2; i > p; --i) {
                at(i) = at(i - 1);
            }
            at(p) = elem{ std::move(id), due };
        }
        else if (l < n) {   // no hole: shift into the room at the tail
            for (size_type i = l; i > p; --i) {
                at(i) = at(i - 1);
            }
            forth(tail);
            at(p) = elem{ std::move(id), due };
        }
        else {      // the ring is full, with holes only before `p`: shift toward the head
            for (size_type i = h3 + 1; i < p; ++i) {
                at(i - 1) = at(i);
            }
            at(p - 1) = elem{ std::move(id), due };
        }
        ++count;
        return true;
    }

    // Nullifies the timer in place; the hole is left for a later start
    int stop(ID id)
    {
        if (count == 0) {
            return -1;
        }
        const size_type l = arc();
        for (size_type i = 0; i < l; ++i) {
            elem& e = at(i);
            if ((e.id == id) && NULIFY{}(e.id)) {
                --count;
                trim();
                return 0;
            }
        }
        return -1;
    }

    // f is called with the ID of each timer that expired, after it is removed,
    // so it may start and stop timers. A timer started from f expires as if
    // started at the moment of the expiry it was started from (as in the list).
    template <class F> void process_expired(U elapsed, F f)
    {
        const U target = now + elapsed;
        for (;;) {
            trim_head();
            if (count == 0) {
                break;
            }
            const elem e = at(0);
            if (e.due > target) {
                break;
            }
            now = e.due;
            forth(head);
            --count;
            f(e.id);
        }
        now = target;
    }

    template <template <class> class V> V<ID> stop_first()
    {
        trim_head();
        if (count == 0) {
            return {};
        }
        const ID id = at(0).id;
        forth(head);
        --count;
        trim_head();
        return V<ID>(id);
    }

private:
    // A hole is a nullified ID; the nulifier says if it was live (and nullifies
    // the copy, which is then thrown away)
    static bool live(ID const& id)
    {
        ID t = id;
        return NULIFY{}(t);
    }

    elem& at(size_type off) { return d.data()[(static_cast<size_type>(head.get()) + off) % B::size()]; }

    // How many slots the ring takes. When it is full, head == tail, as when it
    // is empty; only a full ring has a count of the size.
    size_type arc() const
    {
        const size_type n = B::size();
        const size_type l = (static_cast<size_type>(tail.get()) + n - static_cast<size_type>(head.get())) % n;
        return ((l == 0) && (count > 0)) ? n : l;
    }

    void forth(slot& s) { s.advance_wrap(); }
    void back(slot& s) { s.be((static_cast<size_type>(s.get()) + B::size() - 1) % B::size()); }

    // Holes at the edges of the ring are of no use: drop them
    void trim_head()
    {
        if (count == 0) {
            tail = head;
            return;
        }
        while (!live(at(0).id)) {
            forth(head);
        }
    }
    void trim()
    {
        trim_head();
        if (count == 0) {
            return;
        }
        while (!live(at(arc() - 1).id)) {
            back(tail);
        }
    }
};

template <class ID, int N, class U, class NULIFY>
using cobtmrleanring = bndtmrleanring<ID, cobbound<N>, U, NULIFY>;


#endif // !defined(INC_COBTMRLEANRING)
