#if !defined(INC_optunique)
#define INC_optunique


#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>


// Sole ownership, may be null. Move-only. There is no way to reach the object
// without checking.
template <class T> class optunique {
    std::unique_ptr<T> p;

public:
    optunique(std::unique_ptr<T>&& x) : p(std::move(x)) {}
    optunique(std::nullptr_t) {}

    template <class... A> static optunique make(A&&... a)
    {
        return optunique(std::make_unique<T>(std::forward<A>(a)...));
    }

    optunique(optunique&&) = default; // a moved-from optunique is null, which is valid
    optunique& operator=(optunique&&) = default;

    explicit operator bool() const { return static_cast<bool>(p); }
    void reset() { p.reset(); }
    // Gives the pointer away, leaves null
    std::unique_ptr<T> release() { return std::move(p); }

    template <class F> auto and_then(F&& f)
    {
        return call<T&>(p.get(), std::forward<F>(f));
    }
    template <class F> auto and_then(F&& f) const
    {
        return call<T const&>(p.get(), std::forward<F>(f));
    }

private:
    template <class A, class P, class F> static auto call(P* q, F&& f)
    {
        using ReturnType = std::invoke_result_t<F, A>;
        if constexpr (std::is_void_v<ReturnType>) {
            if (q) {
                std::invoke(std::forward<F>(f), *q);
            }
            return;
        }
        else {
            using R = std::remove_cvref_t<ReturnType>;
            if (q) {
                return R(std::invoke(std::forward<F>(f), *q));
            }
            else {
                return R{};
            }
        }
    }
};


#endif // !defined(INC_optunique)
