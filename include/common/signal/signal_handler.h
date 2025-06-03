#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <string>

class SignalHandler {
public:
    // 捕获当前堆栈跟踪
    static void Capture(int signum);
    
    // 安装全局异常/信号处理
    static void InstallGlobalHandlers();
};

// 自动安装处理器（可选）
#define SIGNALHANDLER_AUTO_INSTALL
#ifdef SIGNALHANDLER_AUTO_INSTALL
namespace {
    struct AutoInstaller {
        AutoInstaller() {
            printf("SignalHandler: Auto-installing global handlers.\n");
            SignalHandler::InstallGlobalHandlers();
        }
    };
    static AutoInstaller __stacktrace_auto_install;
}
#endif

#endif // SIGNAL_HANDLER_H