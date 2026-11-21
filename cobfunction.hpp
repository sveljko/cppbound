/* Compile time bound callable. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBFUNCTION)
#define      INC_COBFUNCTION


#include "cobbound.hpp"

#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>


// Like `std::function`, but what it holds is kept in the object itself, in
// as many bytes as the bound says, so nothing is ever allocated for it. What
// does not fit is not taken: where the size is known at compile time, that
// does not compile.
// Calling one that holds nothing is not an error: nothing is called.
//
// With `COPYABLE` being `true`, it can be copied, so what it holds has to be
// such that it can be copied; "moving" it copies it. With `false`, it can
// only be moved - which leaves what it was moved from holding nothing - and
// so can hold also what can only be moved.
template <class S, class B, bool COPYABLE = true> struct bndfunction;

template <class R, class... A, class B, bool COPYABLE>
struct bndfunction<R(A...), B, COPYABLE> {
    using result_type = R;

    // Can this hold a `D`, as far as what `D` is goes
    template <class D>
    static constexpr bool takes = std::is_invocable_r_v<R, D&, A...>
                                  && (COPYABLE ? std::is_copy_constructible_v<D> : std::is_move_constructible_v<D>);

    bndfunction() = default;

    // What can be called with `A...` to give `R`, and fits
    template <class F, class D = std::decay_t<F>>
        requires(B::fixed && !std::is_same_v<D, bndfunction> && takes<D> && std::is_constructible_v<D, F>)
    bndfunction(F&& f)
    {
        static_assert(sizeof(D) <= B::size(), "The callable does not fit");
        static_assert(alignof(D) <= alignof(std::max_align_t), "The callable is aligned too strictly");
        take(std::forward<F>(f));
    }
    bndfunction(bndfunction const& x)
        requires COPYABLE
    {
        copy(x);
    }
    bndfunction& operator=(bndfunction const& x)
        requires COPYABLE
    {
        if (this != &x) {
            clear();
            copy(x);
        }
        return *this;
    }
    bndfunction(bndfunction&& x) noexcept
        requires(!COPYABLE)
    {
        steal(x);
    }
    bndfunction& operator=(bndfunction&& x) noexcept
        requires(!COPYABLE)
    {
        if (this != &x) {
            clear();
            steal(x);
        }
        return *this;
    }
    ~bndfunction() { clear(); }

    // false, with this as it was, if `f` does not fit. A null pointer (to a
    // function or to a member) leaves this holding nothing.
    template <class F, class D = std::decay_t<F>>
        requires(!std::is_same_v<D, bndfunction> && takes<D> && std::is_constructible_v<D, F>)
    bool assign(F&& f)
    {
        if ((sizeof(D) > B::size()) || (alignof(D) > alignof(std::max_align_t))) {
            return false;
        }
        clear();
        take(std::forward<F>(f));
        return true;
    }

    // Does it hold something to call?
    explicit operator bool() const noexcept { return op != nullptr; }
    bool     empty() const noexcept { return op == nullptr; }
    void     clear()
    {
        if (op != nullptr) {
            op->destroy(d.data());
            op = nullptr;
        }
    }
    // How many bytes what it holds can take
    static auto capacity() { return B::size(); }

    // Calls what it holds. If it holds nothing: does nothing and, if there
    // is something to return, returns `R{}`.
    R operator()(A... a) const
    {
        if (op == nullptr) {
            return R();
        }
        return op->call(d.data(), std::forward<A>(a)...);
    }
    // Calls what it holds, and tells if it did: `true`, or the result; if it
    // holds nothing, `false`, or nothing.
    bool call(A... a) const
        requires std::is_void_v<R>
    {
        if (op == nullptr) {
            return false;
        }
        op->call(d.data(), std::forward<A>(a)...);
        return true;
    }
    template <template <class> class V> V<R> call(A... a) const
    {
        return (op == nullptr) ? V<R>{} : V<R>(op->call(d.data(), std::forward<A>(a)...));
    }

private:
    // What to do with what is held, whatever its type
    struct ops {
        R (*call)(unsigned char*, A&&...);
        void (*copy)(unsigned char*, unsigned char const*);
        void (*move)(unsigned char*, unsigned char*);       // and destroys what it moved from
        void (*destroy)(unsigned char*);
    };
    template <class D>
    static constexpr ops ops_of = {
        [](unsigned char* p, A&&... a) -> R {
            if constexpr (std::is_void_v<R>) {
                std::invoke(*std::launder(reinterpret_cast<D*>(p)), std::forward<A>(a)...);
            }
            else {
                return std::invoke(*std::launder(reinterpret_cast<D*>(p)), std::forward<A>(a)...);
            }
        },
        [](unsigned char* to, unsigned char const* from) {
            if constexpr (std::is_copy_constructible_v<D>) {
                ::new (static_cast<void*>(to)) D(*std::launder(reinterpret_cast<D const*>(from)));
            }
        },
        [](unsigned char* to, unsigned char* from) {
            D* was = std::launder(reinterpret_cast<D*>(from));
            ::new (static_cast<void*>(to)) D(std::move(*was));
            was->~D();
        },
        [](unsigned char* p) { std::launder(reinterpret_cast<D*>(p))->~D(); }
    };

    // Holds nothing when called
    template <class F, class D = std::decay_t<F>> void take(F&& f)
    {
        // A function given by its name is there; a pointer may be null
        using given = std::remove_cvref_t<F>;
        if constexpr (std::is_pointer_v<given> || std::is_member_pointer_v<given>) {
            if (f == nullptr) {
                return;
            }
        }
        ::new (static_cast<void*>(d.data())) D(std::forward<F>(f));
        op = &ops_of<D>;
    }
    void copy(bndfunction const& x)
    {
        if (x.op != nullptr) {
            x.op->copy(d.data(), x.d.data());
            op = x.op;
        }
    }
    // Leaves `x` holding nothing
    void steal(bndfunction& x)
    {
        if (x.op != nullptr) {
            x.op->move(d.data(), x.d.data());
            op   = std::exchange(x.op, nullptr);
        }
    }

    // Changed by a call, if what is held changes itself when called
    alignas(std::max_align_t) mutable typename B::template buffer<unsigned char> d;
    ops const*                                                                  op = nullptr;
};

// `BYTES` is how much room there is for what is to be called; the default
// is enough for a function, a member function and an object to call it on,
// or a lambda that captures a few pointers
template <class S, int BYTES = 3 * sizeof(void*)> using cobfunction = bndfunction<S, cobbound<BYTES>>;

// The one that can only be moved, and so holds also what can only be moved
template <class S, int BYTES = 3 * sizeof(void*)>
using cobmovefunction = bndfunction<S, cobbound<BYTES>, false>;


#endif // !defined(INC_COBFUNCTION)
