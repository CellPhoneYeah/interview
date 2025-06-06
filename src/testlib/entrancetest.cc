#include <thread>
#include <string>
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>
#include <csignal>

#include "spdlog/slog.h"
#include "ellnet/epoll_net.h"
// #include "common/signal_handler.h"

std::string host = "127.0.0.1";
int port = 8088;
ellnet::EpollNet *instance = ellnet::EpollNet::GetInstance();
std::mutex g_mutex;
bool running = true;

void SendMsg(const int sessionId)
{
    SPDLOG_INFO("thread to send msg {}", sessionId);
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    
    try
    {
        SPDLOG_INFO("thread to send msg ");
        instance->StartConnect(sessionId);
        // std::this_thread::sleep_for(std::chrono::seconds(3));
        ellnet::EpollNet *instance = ellnet::EpollNet::GetInstance();
        int msgId = 1;
        for (;;)
        {
            if(!running){
                SPDLOG_INFO("test thread {} finished !!!", sessionId);
                return;
            }
            std::stringstream ss;
            ss << "hello i am "  << sessionId << " send msg Id: " << msgId++;
            std::string newstr = ss.str();
            SPDLOG_INFO("thread to send msg {}", newstr);
            instance->SendMsg(newstr, sessionId);
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        SPDLOG_INFO("test thread {} finished !!!", sessionId);
    }
    catch (const std::exception &e)
    {
        SPDLOG_ERROR("SendMsg failed {}", e.what());
    }
}

// 全局异常处理函数
void myTerminateHandler() {
    try {
        // 尝试获取当前异常
        if (std::current_exception()) {
            std::rethrow_exception(std::current_exception());
        }
    } catch (const std::exception& e) {
        std::cerr << "Uncaught exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown uncaught exception" << std::endl;
    }
    
    // 打印堆栈信息
    std::cerr << "Program terminated at: " << __FILE__ << ":" << __LINE__ << std::endl;
    
    // 保持原有的终止行为
    std::abort();
}

volatile sig_atomic_t g_signal_received = 0;

void signal_handler(int signal)
{
    SPDLOG_INFO("handler signal {} ...", signal);
    if(running){
        SPDLOG_INFO("stop running ...");
        std::lock_guard<std::mutex> lock(g_mutex);
        running = false;
    }
    ellnet::EpollNet * p_net = ellnet::EpollNet::GetInstance();
    p_net->close();
}

void clean_up()
{
    SPDLOG_INFO("Cleaning up...");
    if(running){
        SPDLOG_INFO("stop running ...");
        std::lock_guard<std::mutex> lock(g_mutex);
        running = false;
    }
    ellnet::EpollNet * p_net = ellnet::EpollNet::GetInstance();
    p_net->close();
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGPIPE, SIG_IGN); // 忽略SIGPIPE信号
    std::set_terminate(
        []()
        {
            clean_up();
            std::abort();
        });
    std::vector<std::thread> threads;
    try
    {
        SPDLOG_INFO("connect to {}:{}", host, port);
        for (int i = 0; i < 1; i++)
        {
            const int sessionId = instance->ConnectTo(host, port);
            if (sessionId <= 0)
            {
                SPDLOG_INFO("connect to {}:{} failed", host, port);
                return -1;
            }
            else
            {
                SPDLOG_INFO("start send Msg thread {}", sessionId);
                std::thread th(SendMsg, sessionId);
                threads.push_back(std::move(th));
            }
        }
        instance->JoinThread();
        for(int i = 0; i < threads.size(); i++){
            threads[i].join();
        }
    }
    catch (const std::exception& e)
    {
        SPDLOG_ERROR("test err {}", e.what());
    }
    SPDLOG_INFO("test finished !!!");

    return 0;
}