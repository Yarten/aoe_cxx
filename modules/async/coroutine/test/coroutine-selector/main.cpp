//
// Created by yarten on 24-1-3.
//

#include <string>
#include <aoe/async/coroutine/selector.h>

class A
{
public:
    bool isReady() { return true; }
};

class B
{
public:
    bool isReady() { return false; }
};


int main()
{
    aoe::async::coroutine::Selector select {A(), B()};

    select.isReady();


    return 0;
}
