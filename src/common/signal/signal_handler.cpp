#include "signal_handler.h"
#include <csignal>
#include <execinfo.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>

// 信号名称映射
const char* signalNames[] = {
    "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
    "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1",
    "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM"
};

// 从调用栈地址获取文件名和行号
void getSourceLocation(const char* executable, const char* address, char* buffer, size_t bufferSize) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "addr2line -e %s %s", executable, address);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        snprintf(buffer, bufferSize, "Error: popen() failed");
        return;
    }
    
    if (fgets(buffer, bufferSize, pipe) == NULL) {
        snprintf(buffer, bufferSize, "Unknown location");
    }
    
    // 移除换行符
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    pclose(pipe);
}

void SignalHandler::Capture(int signum) {
    // 基本信号信息
    char timestamp[32];
    time_t now = time(nullptr);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    char signalInfo[128];
    snprintf(signalInfo, sizeof(signalInfo), 
             "=== %s: Caught signal %d (%s) in process %d ===\n", 
             timestamp, signum, 
             (signum > 0 && signum <= 15) ? signalNames[signum-1] : "Unknown",
             getpid());
    
    // 输出到控制台
    write(STDERR_FILENO, signalInfo, strlen(signalInfo));
    
    // 输出到文件
    FILE* stackFile = fopen("stacktrace.log", "a");
    if (stackFile) {
        fwrite(signalInfo, 1, strlen(signalInfo), stackFile);
    }
    
    // 获取调用栈
    void *array[20]; // 增加栈深度
    size_t size = backtrace(array, 20);
    
    // 转换调用栈为字符串
    char **strings = backtrace_symbols(array, size);
    
    if (strings) {
        // 获取当前可执行文件路径
        char executable[256];
        readlink("/proc/self/exe", executable, sizeof(executable) - 1);
        
        // 输出到控制台
        write(STDERR_FILENO, "Stack trace:\n", 14);
        if (stackFile) {
            fwrite("Stack trace:\n", 1, 14, stackFile);
        }
        
        for (size_t i = 0; i < size; i++) {
            // 输出原始栈帧
            char frameInfo[512];
            snprintf(frameInfo, sizeof(frameInfo), "#%zu %s\n", i, strings[i]);
            write(STDERR_FILENO, frameInfo, strlen(frameInfo));
            if (stackFile) {
                fwrite(frameInfo, 1, strlen(frameInfo), stackFile);
            }
            
            // 解析地址以获取源代码位置
            char* addrStart = strchr(strings[i], '[');
            char* addrEnd = strchr(strings[i], ']');
            if (addrStart && addrEnd && addrEnd > addrStart) {
                *addrEnd = '\0';
                char* addr = addrStart + 1;
                
                char sourceLocation[512];
                getSourceLocation(executable, addr, sourceLocation, sizeof(sourceLocation));
                
                char sourceInfo[512];
                snprintf(sourceInfo, sizeof(sourceInfo), "    at %s\n", sourceLocation);
                write(STDERR_FILENO, sourceInfo, strlen(sourceInfo));
                if (stackFile) {
                    fwrite(sourceInfo, 1, strlen(sourceInfo), stackFile);
                }
            }
        }
        
        free(strings);
    }
    
    // 关闭文件
    if (stackFile) {
        fwrite("========================================\n\n", 1, 42, stackFile);
        fclose(stackFile);
    }
    
    // 恢复默认信号处理
    signal(signum, SIG_DFL);
    // 重新发送信号以触发默认行为
    raise(signum);
}

void SignalHandler::InstallGlobalHandlers() {
    static bool installed = false;
    if (installed) return;
    
    // 安装信号处理函数
    signal(SIGSEGV, Capture);
    signal(SIGABRT, Capture);
    signal(SIGFPE, Capture);
    signal(SIGILL, Capture);
    signal(SIGBUS, Capture);
    
    installed = true;
}
    