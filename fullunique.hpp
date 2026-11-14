#if !defined(INC_fullunique)
#define INC_fullunique


#include <memory>
#include <utility>


// Sole ownership, never null. A unique pointer can not be moved without
// leaving a null behind, so this is neither copyable nor movable: it lives
// where it was made.
template <class T> class fullunique {
    std::unique_ptr<T> p;
    explicit fullunique(std::unique_ptr<T> x) : p(std::move(x)) {}

public:
    template <class... A> static fullunique make(A&&... a)
    {
        return fullunique(std::make_unique<T>(std::forward<A>(a)...));
    }

    fullunique(fullunique const&) = delete;
    fullunique& operator=(fullunique const&) = delete;

    T&       operator*() { return *p; }
    T const& operator*() const { return *p; }
    T*       operator->() { return p.get(); }
    T const* operator->() const { return p.get(); }
};


#endif // !defined(INC_fullunique)
