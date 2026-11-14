#if !defined(INC_optptr)
#define INC_optptr


#include "fullptr.hpp"

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>


template<class T> struct optptr {
    constexpr optptr(T *x) : p(x) {}
    constexpr optptr(std::nullptr_t) : p(nullptr) {}
    constexpr optptr(fullptr<T> x) : p(x.get()) {}

    constexpr explicit operator bool() const { return p != nullptr; }

    constexpr fullptr<T> value_or(fullptr<T> x) const { return p ? fullptr<T>{ p } : x; }

    template<class F> constexpr auto or_else(F&& f) { return p ? *this : std::forward<F>(f)(); }
    template<class F> constexpr auto or_else(F&& f) const { return p ? *this : std::forward<F>(f)(); }

    template<class F> constexpr auto and_then(F&& f) {
        using ReturnType = std::invoke_result_t<F, T&>;
        
        if constexpr (std::is_void_v<ReturnType>) {
            if (p) {
                std::invoke(std::forward<F>(f), *p);
            }
            return;
        } 
        else {        
            using R = std::remove_cvref_t<ReturnType>;
            if (p)
                return R(std::invoke(std::forward<F>(f), *p));
            else
                return R{};
        }
    }
    template<class F> constexpr auto and_then(F&& f) const {
        using ReturnType = std::invoke_result_t<F, T const&>;
        T const* cp = p;
        
        if constexpr (std::is_void_v<ReturnType>) {
            if (cp) {
                std::invoke(std::forward<F>(f), *cp);
            }
            return;
        } 
        else {        
            using R = std::remove_cvref_t<ReturnType>;
            if (cp)
                return R(std::invoke(std::forward<F>(f), *cp));
            else
                return R{};
        }
    }

    constexpr void reset() { p = nullptr; }
    constexpr auto release() { auto _ = p; p = nullptr; return _; }

private:
    T* p = nullptr;
};


#endif // !defined(INC_optptr)
