#if !defined(INC_perhaps)
#define INC_perhaps

#include <functional>
#include <cstddef> // byte


template <class T>
class perhaps {
    alignas(T) std::byte v[sizeof(T)];
    bool has = false;

public:
    perhaps() = default;

    template <class U>
    static auto unit(U&& x) {
        return perhaps<std::decay_t<U>>(std::forward<U>(x));
    }

    template <class... Args> constexpr perhaps(Args&&... args) {
        new (v) T(args...);
        has = true;
    }

    constexpr perhaps(const perhaps& mx) {
        mx.and_then([&](auto x) {
            new (v) T(x);
            has = true;
        });
    }

    constexpr perhaps(perhaps&& mx) noexcept {
        mx.and_then([&](auto& x) {
            new (v) T(std::move(x));
            has = true;
        });
    }

    constexpr bool has_value() const { return has; }

    template <class F> constexpr auto and_then(F&& f) & {
        if (has) {
            return std::invoke(std::forward<F>(f), *reinterpret_cast<T*>(v));
        }
        else {
            return std::remove_cvref_t<std::invoke_result_t<F, T&>>{};
        }
    }
    template <class F> constexpr auto and_then(F&& f) const& {
        if (has) {
            return std::invoke(std::forward<F>(f), *reinterpret_cast<T const*>(v));
        }
        else {
            return std::remove_cvref_t<std::invoke_result_t<F, const T&>>{};
        }
    }
    template <class F> constexpr auto and_then(F&& f) && {
        if (has) {
            return std::invoke(std::forward<F>(f), std::move(*reinterpret_cast<T*>(v)));
        }
        else {
            return std::remove_cvref_t<std::invoke_result_t<F, T&>>{};
        }
    }
    template <class F> constexpr auto and_then(F&& f) const&& {
        if (has) {
            return std::invoke(std::forward<F>(f), std::move(*reinterpret_cast<T const*>(v)));
        }
        else {
            return std::remove_cvref_t<std::invoke_result_t<F, const T&>>{};
        }
    }

    template <class F> constexpr auto or_else(F&& f) const& {
        return has ? *this : std::forward<F>(f)();
    }
    template <class F> constexpr auto or_else(F&& f) && {
        return has ? std::move(*this) : std::forward<F>(f)();
    }

    template<typename F> constexpr perhaps<std::invoke_result_t<F, const T&>> transform(F&& func) const {
        using U = std::invoke_result_t<F, const T&>;
        if (has) {
            return {std::invoke(std::forward<F>(func), *reinterpret_cast<T const*>(v))};
        }
        return {};
    }

    template<class U = std::remove_cv_t<T>> constexpr T value_or(U &&x) const& {
        return has ? *reinterpret_cast<T const*>(v) : std::forward<U>(x);
    }
    template<class U = std::remove_cv_t<T>> constexpr T value_or(U &&x) && {
        return has ? std::move(*reinterpret_cast<T*>(v)) : std::forward<U>(x);
    }

    constexpr void reset() noexcept {
        if (has) {
            reinterpret_cast<T*>(v)->~T();
            has = false;
        }
    }
};


#endif // !defined(INC_perhaps)
