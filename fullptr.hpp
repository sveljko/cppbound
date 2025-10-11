#if !defined(INC_fullptr)
#define INC_fullptr


#include <optional>


template<class T> struct fullptr {
    template <template <class> class V> static V<fullptr> make(T* p) {
        if (p) { return fullptr{p}; } else { return {}; }
    }

    fullptr(fullptr const& x) : p(x.p) {}

    fullptr(fullptr const&& x) : p(x.p) {}

    ~fullptr() {}

    T* get() { return p; }
    T const* get() const { return p; }

    T* operator->() { return p;}
    T const* operator->() const { return p;}

    T& operator*() { return *p;}
    T const& operator*() const { return *p;}

private:
    fullptr(T*x) : p(x) {}
    fullptr() = delete;

    T* p;
};

template <template <class> class V, class T> inline V<fullptr<T>> make_fullptr(T* p) {
    return fullptr<T>::template make<V>(p);
}

template <class T> inline std::optional<fullptr<T>> make_fullptr(T* p) {
    return fullptr<T>::template make<std::optional>(p);
}


#endif // !defined(INC_fullptr)
