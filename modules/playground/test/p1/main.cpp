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
    }

    constexpr ~MyVector()
    {
        clear();
    }

    constexpr size_t size() const
    {
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

int main()
{
    constexpr size_t s = g(262144);

    std::cout << s << std::endl;


    return 0;
}
