#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H
#include <csignal>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <libunwind.h>
#include <cxxabi.h>

namespace ellnet
{
    class SignalHandler
    {
    private:
        static void PrintStackTrace()
        {
            unw_cursor_t cursor;
            unw_context_t context;

            // 初始化游标和上下文
            unw_getcontext(&context);
            unw_init_local(&cursor, &context);

            std::cerr << "Stack trace:" << std::endl;

            // 遍历堆栈帧
            while (unw_step(&cursor) > 0)
            {
                unw_word_t offset, pc;
                char sym[256];

                // 获取当前指令指针（IP）和函数名
                unw_get_reg(&cursor, UNW_REG_IP, &pc);
                if (pc == 0)
                    break;

                // 解析函数名
                if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
                {
                    // 解码C++符号（demangle）
                    char *demangled = abi::__cxa_demangle(sym, nullptr, nullptr, nullptr);
                    const char *name = demangled ? demangled : sym;

                    std::cerr << "0x" << std::hex << pc << ": " << name << "+0x" << offset << std::endl;

                    if (demangled)
                        free(demangled);
                }
                else
                {
                    std::cerr << "0x" << std::hex << pc << ": ??" << std::endl;
                }
            }
        }
        static void SignalHandlerFunc(int signal)
        {
            // 打印信号信息
            std::cerr << "Received signal: " << signal << std::endl;

            // 打印堆栈跟踪（需后续实现）
            PrintStackTrace();

            // 退出程序
            exit(1);
        }

        struct sigaction sa;
    public:
        SignalHandler()
        {
            sa.sa_handler = SignalHandlerFunc; // 绑定处理函数
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = SA_RESTART;
            // 注册需要捕获的信号
            sigaction(SIGABRT, &sa, nullptr);
            sigaction(SIGSEGV, &sa, nullptr);
            sigaction(SIGFPE, &sa, nullptr);
            sigaction(SIGILL, &sa, nullptr);
        }
    };
    static SignalHandler signal_handler_;
}

#endif