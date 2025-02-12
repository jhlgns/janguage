#pragma once

#include <utility>

template<typename F>
struct ScopeExit
{
    F f;
    ScopeExit(F f)
        : f(f)
    {
    }
    ~ScopeExit() { f(); }
};

struct DEFER_TAG
{
};

template<class F>
ScopeExit<F> operator+(DEFER_TAG, F &&f)
{
    return ScopeExit<F>{std::forward<F>(f)};
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define defer auto DEFER_2(ScopeExit, __LINE__) = DEFER_TAG{} + [&]()
