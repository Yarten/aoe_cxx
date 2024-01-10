//
// Created by yarten on 23-12-22.
//

#include <vector>
#include <type_traits>
#include <iostream>
#include <concurrentqueue.h>


constexpr auto f()
{
    std::vector<int> v(1, 3);
    return v.size();
}

template<class T>
concept MyConcept = std::is_same<T, int>::value;

class MyVector
{
public:
    constexpr MyVector(size_t n)
        : n_(n)
    {
        ptr_ = aloc_.allocate(n_);

        for (size_t idx = 0; idx < n_; ++idx)
            ptr_[idx] = static_cast<int>(idx);

        std::allocator<std::byte> alloc;
        alloc.deallocate(alloc.allocate(100), 100);
    }

    constexpr ~MyVector()
    {
        clear();
    }

    constexpr size_t size() const
    {
        if (ptr_ == nullptr)
            return 0;

        size_t sum = ptr_[0];

        for (size_t idx = 0; idx < n_; ++idx)
            sum += ptr_[idx];

        return sum;
    }

    constexpr void clear()
    {
        if (ptr_ == nullptr)
            return;

        aloc_.deallocate(ptr_, n_);
        ptr_ = nullptr;
        n_   = 0;
    }

private:
    std::allocator<int> aloc_;
    int * ptr_ = nullptr;
    size_t n_ = 0;
};

constexpr auto g(size_t n)
{
    MyVector vec(n);
    size_t s = vec.size();

    vec.clear();
    s += vec.size();

    return s;
}

template<class TDerived>
class Base
{
public:
    void f()
    {
        auto * self = static_cast<TDerived*>(this);

        auto addr_dirr = []<class T, class E>(const T * lhs, const E * rhs) -> std::size_t
        {
            return reinterpret_cast<std::size_t>(lhs) - reinterpret_cast<std::size_t>(rhs);
        };

        std::cout
            << addr_dirr(&x, self) << " " << offsetof(Base, x) << std::endl
            << addr_dirr(&y, self) << " " << offsetof(Base, y) << std::endl
            << addr_dirr(&self->z, self) << " " << offsetof(TDerived, z) << std::endl
            << addr_dirr(&self->a, self) << " " << offsetof(TDerived, a) << std::endl;
    }

    struct Inner
    {
        void g(TDerived * self)
        {
            self->u;
        }
    };

    void g()
    {
        static_cast<TDerived*>(this)->u;

        Inner inner;
        inner.g(static_cast<TDerived*>(this));
    }

protected:
    int x = 0;
    int y = 0;
};

class Derived : public Base<Derived>
{
public:
    int z = 0;
    int a = 0;

    struct A
    {
        constexpr static  inline int x = 0;

        constexpr int operator[](int x) const
        {
            return x;
        }
    } static inline obj;

private:
    double u = 1.0;
    friend class Base;
};


int main()
{
    constexpr size_t s = g(262144);

    std::cout << s << std::endl;

    std::atomic<int> m;

    Derived b;
    b.f();
    b.g();

    static_assert(Derived::obj[0] == 0);

    return 0;
}
