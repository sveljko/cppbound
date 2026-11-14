#if !defined(INC_perhaps)
#define INC_perhaps

#include <functional>
#include <cstddef> // byte
#include <new>
#include <type_traits>
#include <utility>


template <class T>
class perhaps {
    alignas(T) std::byte v[sizeof(T)];
    bool has = false;

    constexpr T* ptr() { return std::launder(reinterpret_cast<T*>(v)); }
    constexpr T const* ptr() const { return std::launder(reinterpret_cast<T const*>(v)); }

    template <class... Args> constexpr void make(Args&&... args) {
        ::new (static_cast<void*>(v)) T(std::forward<Args>(args)...);
        has = true;
    }

public:
    constexpr perhaps() noexcept {}

    template <class U>
    static auto unit(U&& x) {
        return perhaps<std::decay_t<U>>(std::forward<U>(x));
    }

    // Not for (copying or moving) a `perhaps` itself
    template <class A, class... Args>
        requires(!std::is_same_v<std::remove_cvref_t<A>, perhaps>
                 && std::is_constructible_v<T, A, Args...>)
    constexpr perhaps(A&& a, Args&&... args) {
        make(std::forward<A>(a), std::forward<Args>(args)...);
    }

    constexpr perhaps(const perhaps& mx) requires std::is_copy_constructible_v<T> {
        if (mx.has) {
            make(*mx.ptr());
        }
    }

    constexpr perhaps(perhaps&& mx) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (mx.has) {
            make(std::move(*mx.ptr()));
        }
    }

    constexpr perhaps& operator=(const perhaps& mx) requires std::is_copy_constructible_v<T> {
        if (this != &mx) {
            reset();
            if (mx.has) {
                make(*mx.ptr());
            }
        }
        return *this;
    }

    constexpr perhaps& operator=(perhaps&& mx) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this != &mx) {
            reset();
            if (mx.has) {
                make(std::move(*mx.ptr()));
            }
        }
        return *this;
    }

    constexpr ~perhaps() { reset(); }

    constexpr bool has_value() const { return has; }

    template <class F> constexpr auto and_then(F&& f) & {
        using R = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        if constexpr (std::is_void_v<R>) {
            if (has) std::invoke(std::forward<F>(f), *ptr());
        } else {
            if (has) return R(std::invoke(std::forward<F>(f), *ptr()));
            else     return R{};
        }
    }
    template <class F> constexpr auto and_then(F&& f) const& {
        using R = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if constexpr (std::is_void_v<R>) {
            if (has) std::invoke(std::forward<F>(f), *ptr());
        } else {
            if (has) return R(std::invoke(std::forward<F>(f), *ptr()));
            else     return R{};
        }
    }
    template <class F> constexpr auto and_then(F&& f) && {
        using R = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if constexpr (std::is_void_v<R>) {
            if (has) std::invoke(std::forward<F>(f), std::move(*ptr()));
        } else {
            if (has) return R(std::invoke(std::forward<F>(f), std::move(*ptr())));
            else     return R{};
        }
    }
    template <class F> constexpr auto and_then(F&& f) const&& {
        using R = std::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        if constexpr (std::is_void_v<R>) {
            if (has) std::invoke(std::forward<F>(f), std::move(*ptr()));
        } else {
            if (has) return R(std::invoke(std::forward<F>(f), std::move(*ptr())));
            else     return R{};
        }
    }
    template <class F> constexpr auto or_else(F&& f) const& {
        return has ? *this : std::forward<F>(f)();
    }
    template <class F> constexpr auto or_else(F&& f) && {
        return has ? std::move(*this) : std::forward<F>(f)();
    }

    template<typename F>
    constexpr perhaps<std::remove_cvref_t<std::invoke_result_t<F, const T&>>> transform(F&& func) const {
        if (has) {
            return {std::invoke(std::forward<F>(func), *ptr())};
        }
        return {};
    }

    template<class U = std::remove_cv_t<T>> constexpr T value_or(U &&x) const& {
        return has ? *ptr() : static_cast<T>(std::forward<U>(x));
    }
    template<class U = std::remove_cv_t<T>> constexpr T value_or(U &&x) && {
        return has ? std::move(*ptr()) : static_cast<T>(std::forward<U>(x));
    }

    template <class... A> constexpr void emplace(A&&... a) {
        reset();
        make(std::forward<A>(a)...);
    }

    constexpr bool operator==(perhaps const& x) const {
        return has ? (x.has && (*ptr() == *x.ptr())) : !x.has;
    }

    constexpr void reset() noexcept {
        if (has) {
            ptr()->~T();
            has = false;
        }
    }
};


#endif // !defined(INC_perhaps)
