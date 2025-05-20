#include <thread>
#include <string>
#include <chrono>
#include <iostream>

#include "spdlog/slog.h"
#include "ellnet/epoll_net.h"
#include "common/signal_handler.h"

std::string host = "127.0.0.1";
int port = 8088;

void SendMsg(const int sessionId)
{
    SPDLOG_INFO("thread to send msg {}", sessionId);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    try
    {
        ellnet::EpollNet *instance = ellnet::EpollNet::GetInstance();
        std::string str = "hello i am ";
        str = str.append(std::to_string(sessionId));
        for (;;)
        {
            SPDLOG_INFO("thread to send msg {}", str);
            instance->SendMsg(str, sessionId);
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }
    }
    catch (const std::exception &e)
    {
        SPDLOG_ERROR("SendMsg failed {}", e.what());
    }
}

int main()
{
    try
    {
        ellnet::EpollNet *instance = ellnet::EpollNet::GetInstance();
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
                instance->StartConnect(sessionId);
                std::thread th(SendMsg, sessionId);
            }
        }
        instance->JoinThread();
    }
    catch (const std::exception& e)
    {
        SPDLOG_ERROR("test err {}", e.what());
    }

    return 0;
}