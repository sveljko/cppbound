#if !defined(INC_fullshared)
#define INC_fullshared


#include <memory>
#include <utility>


template <class U> class optshared;

// Shared ownership, never null
template <class T> class fullshared {
    std::shared_ptr<T> p;
    explicit fullshared(std::shared_ptr<T> x) : p(std::move(x)) {}
    template <class U> friend class optshared;

public:
    // A new object. (Out of memory is std::bad_alloc, as with any `new`.)
    template <class... A> static fullshared make(A&&... a)
    {
        return fullshared(std::make_shared<T>(std::forward<A>(a)...));
    }
    // From a pointer that might be null
    template <template <class> class V> static V<fullshared> from(std::shared_ptr<T> x)
    {
        if (x) {
            return fullshared(std::move(x));
        }
        return {};
    }

    // Copies only: a move would leave a null behind. (No move operations
    // are declared, so "moving" copies.)
    fullshared(fullshared const&) = default;
    fullshared& operator=(fullshared const&) = default;

    T& operator*() const { return *p; }
    T* operator->() const { return p.get(); }
    std::shared_ptr<T> shared() const { return p; } // to hand over to other code
    bool operator==(fullshared const&) const = default;
};


#endif // !defined(INC_fullshared)
