#ifndef Signal_HANDLER_TEST
#define Signal_HANDLER_TEST
#include "signal_handler.h"
#include <thread>
#include <stdexcept>

void TriggerSegfault()
{
    // 故意制造段错误
    int *ptr = nullptr;
    *ptr = 42;
}

void ThrowException()
{
    throw std::runtime_error("Test exception from thread");
}

int main()
{
    // 测试1: 线程中触发段错误
    std::thread t1(TriggerSegfault);
    t1.join();

    // 测试2: 线程中抛出未捕获异常
    std::thread t2(ThrowException);
    t2.join();

    // 测试3: 主线程中抛出异常
    throw "Unhandled exception in main thread";

    return 0;
}
#endif // Signal_HANDLER_TEST