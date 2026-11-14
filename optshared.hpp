#if !defined(INC_optshared)
#define INC_optshared


#include "fullshared.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>


// Shared ownership, may be null. There is no way to reach the object
// without checking.
template <class T> class optshared {
    std::shared_ptr<T> p;

public:
    optshared(std::shared_ptr<T> x) : p(std::move(x)) {}
    optshared(std::nullptr_t) {}
    optshared(fullshared<T> x) : p(std::move(x.p)) {}

    explicit operator bool() const { return static_cast<bool>(p); }
    void reset() { p.reset(); }
    std::shared_ptr<T> shared() const { return p; }

    template <template <class> class V> V<fullshared<T>> full() const
    {
        if (p) {
            return fullshared<T>(p);
        }
        return {};
    }

    fullshared<T> value_or(fullshared<T> x) const { return p ? fullshared<T>(p) : x; }

    template <class F> auto or_else(F&& f) const { return p ? *this : std::forward<F>(f)(); }

    // f is called with the object, if there is one
    template <class F> auto and_then(F&& f) const
    {
        using ReturnType = std::invoke_result_t<F, T&>;
        if constexpr (std::is_void_v<ReturnType>) {
            if (p) {
                std::invoke(std::forward<F>(f), *p);
            }
            return;
        }
        else {
            using R = std::remove_cvref_t<ReturnType>;
            if (p) {
                return R(std::invoke(std::forward<F>(f), *p));
            }
            else {
                return R{};
            }
        }
    }
};


#endif // !defined(INC_optshared)
