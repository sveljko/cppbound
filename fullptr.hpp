#if !defined(INC_fullptr)
#define INC_fullptr


#include "perhaps.hpp"

#include <memory>
#include <type_traits>

template <class U> struct optptr;


template<class T> struct fullptr {
    template <template <class> class V> static constexpr V<fullptr> make(T* p) {
        if (p) { return fullptr{p}; } else { return {}; }
    }

    // A reference always refers to an object
    static constexpr fullptr from(T& r) { return fullptr{ std::addressof(r) }; }

    // To a pointer to const, or to a base
    template <class U>
        requires std::is_convertible_v<U*, T*>
    constexpr fullptr(fullptr<U> x) : p(x.get()) {}

    constexpr bool operator==(fullptr const&) const = default;

    constexpr T* get() { return p; }
    constexpr T const* get() const { return p; }

    constexpr T* operator->() { return p;}
    constexpr T const* operator->() const { return p;}

    constexpr T& operator*() { return *p;}
    constexpr T const& operator*() const { return *p;}

private:
    template <class U> friend struct optptr;
    constexpr fullptr(T*x) : p(x) {}
    fullptr() = delete;

    T* p;
};

template <template <class> class V, class T> inline constexpr V<fullptr<T>> make_fullptr(T* p) {
    return fullptr<T>::template make<V>(p);
}

template <class T> inline perhaps<fullptr<T>> make_fullptr(T* p) {
    return fullptr<T>::template make<perhaps>(p);
}


#endif // !defined(INC_fullptr)
