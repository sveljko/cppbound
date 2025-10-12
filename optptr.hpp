#if !defined(INC_optptr)
#define INC_optptr


#include "fullptr.hpp"


template<class T> struct optptr {
    constexpr optptr(T *x) : p(x) {}
    constexpr optpr(nullptr_t) : p(nullptr) {}

    constexpr optptr(optptr const& x) : p(x.p) {}
    constexpr optptr(fullptr const& x) : p(x.get()) {}

    constexpr optptr(optptr const&& x) : p(x.p) {}
    constexpr optptr(fullptr const&& x) : p(x.get()) {}

    constexpr ~optptr() { p = nullptr; }

    operator bool() const { return p != nullptr; }

    constexpr T* value_or(fullptr<T> x) { return p ? p : x.get(); }
    constexpr T const* value_or(fullptr<T const> x) const { return p ? p : x.get(); }

    template<class F> constexpr auto or_else(F&& f) { return p ? *this : std::forward<F>(f)(); }
    template<class F> constexpr auto or_else(F&& f) const { return p ? *this : std::forward<F>(f)(); }

    template<class F> constexpr auto and_then(F&& f) {
        if (p)
            return std::invoke(std::forward<F>(f), p);
        else
            return std::remove_cvref_t<std::invoke_result_t<F, T&>>{};
    }
    template<class F> constexpr auto and_then(F&& f) const {
        if (p)
            return std::invoke(std::forward<F>(f), p);
        else
            return std::remove_cvref_t<std::invoke_result_t<F, T const&>>{};
    }

    void reset() { p = nullptr; }

private:
    T* p = nullptr;
};


#endif // !defined(INC_fullptr)
