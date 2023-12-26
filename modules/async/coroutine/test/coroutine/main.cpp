//
// Created by yarten on 23-12-26.
//

#include <aoe/async/coroutine.h>
#include <iostream>
#include <vector>


template<class T>
class A;

template<class T>
class B
{
public:
    void f(A<T> a);
};

template<class T>
class A
{
public:
    void g() {}
};

template<class T>
void B<T>::f(A<T> a)
{
    a.g();
}


int main()
{
    for(int i : std::vector<int>(3, 1))
    {
        std::cout << i << std::endl;
    }

    B<int> a;
    a.f(A<int>());

    return 0;
}
