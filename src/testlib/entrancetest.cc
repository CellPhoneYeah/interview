#include <thread>
#include <string>
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>

#include "spdlog/slog.h"
#include "ellnet/epoll_net.h"
// #include "common/signal_handler.h"

std::string host = "127.0.0.1";
int port = 8088;
ellnet::EpollNet *instance = ellnet::EpollNet::GetInstance();

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

int main()
{
    std::set_terminate(myTerminateHandler);
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