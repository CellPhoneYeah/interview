#include <unistd.h>
#include <execinfo.h>

#include <thread>
#include <vector>
#include <ctime>
#include <chrono>
#include <stdexcept>
#include <iostream>

#include "message_queue/global_queue.h"
#include "ellnet/epoll_net.h"
// #include "signal_handler.h"

namespace ellnet
{
    void print_stacktrace() {
        const int max_frames = 64;
        void* buffer[max_frames];
        int frames = backtrace(buffer, max_frames);
        char** symbols = backtrace_symbols(buffer, frames);
        
        std::cerr << "=== Stack Trace ===" << std::endl;
        for (int i = 0; i < frames; ++i) {
            std::cerr << symbols[i] << std::endl;
        }
        std::cerr << "===================" << std::endl;
        
        free(symbols);
    } 
    static std::string kListenHost = "127.0.0.1";
    static int kListenPort = 8088;
    int worker_count = 2;
    int producer_count = 1;
    int mq_count = 1;

    void NetWorker()
    {
        try{
            EpollNet * p_net = EpollNet::GetInstance();
            SPDLOG_INFO("entrance worker listen on {}:{}", kListenHost, kListenPort);
            const int sessionId = p_net->ListenOn(kListenHost, kListenPort);
            SPDLOG_INFO("entrance worker listen {}", sessionId);
            if(sessionId > 0){
                SPDLOG_INFO("start listen {}:{}", kListenHost, kListenPort);
                p_net->StartListen(sessionId);
            }else{
                SPDLOG_INFO("listen failed {}:{}", kListenHost, kListenPort);
            }
            p_net->JoinThread();
        }catch(const std::exception e){
            SPDLOG_ERROR("listen failed {}", e.what());
            print_stacktrace();
        }
    }

    void Worker(int id)
    {
        SPDLOG_INFO("start worker {}", id);
        GlobalQueue *gq = GlobalQueue::GetInstance();
        std::shared_ptr<MessageQueue> mq;
        int handle_count = 0;
        time_t last_show_time = std::time(nullptr);
        for (;;)
        {
            // sleep(5);
            mq = gq->PopMQHasMsg();
            time_t now = std::time(nullptr);
            if (mq != nullptr)
            {
                if (mq->HasMsg())
                {
                    std::string msg = mq->PopM();
                    handle_count++;
                    if(handle_count >= INT32_MAX){
                        handle_count = 0;
                    }
                }
                gq->BackMQ(mq);
                // SPDLOG_INFO("{} worker handle msg:{}", id, handle_count);
            }
            else
            {
                // SPDLOG_INFO("{} worker wait producer do nothing count:{}", id, handle_count);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            if (std::difftime(now, last_show_time) > 10)
            {
                last_show_time = now;
                SPDLOG_INFO("worker :{} count:{} ", id, handle_count);
            }
        }
    }

    void Producer(int producerId)
    {
        SPDLOG_INFO("start producer {}", producerId);
        GlobalQueue *gq = GlobalQueue::GetInstance();
        time_t last_show_time = std::time(nullptr);
        int sendCount = 0;
        for (;;)
        {
            // sleep(5);
            std::srand(std::time(nullptr));
            int randomId = std::rand() % mq_count;
            std::string msg = "msg from producer" + std::to_string(producerId);
            time_t now = std::time(nullptr);
            if (gq->SendMsg(randomId, msg) < 0)
            {
                // SPDLOG_INFO("{} producer wait worker try send to {} failed", producerId, randomId);
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
            else
            {
                sendCount++;
                if(sendCount >= INT32_MAX){
                    sendCount = 0;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                // SPDLOG_INFO("{} producer sent msg success {}", producerId, sendCount);
            }
            if (std::difftime(now, last_show_time) > 10)
            {
                last_show_time = now;
                SPDLOG_INFO("producer {} count:{} send msg ", producerId, sendCount);
            }
        }
    }
}

int main()
{
    // 注册未捕获异常的处理函数
    std::set_terminate(
        []()
        {
            SPDLOG_ERROR("err happended");
            std::cout << "unknow err " << std::endl;
            ellnet::print_stacktrace();
            std::abort();
        });
    try
    {

        // GlobalQueue *gq = GlobalQueue::GetInstance();
        // for (int i = 0; i < ellnet::mq_count; i++)
        // {
        //     SPDLOG_INFO("new mq id {}", i);
        //     gq->NewMQ(i);
        // }

        std::thread net_th = std::thread(ellnet::NetWorker);

        // std::vector<std::thread> workers;
        // std::vector<std::thread> producers;
        // for (int i = 0; i < ellnet::worker_count; i++)
        // {
        //     SPDLOG_INFO("new work id {}", i);
        //     workers.emplace_back(ellnet::Worker, i);
        // }

        // for (int i = 0; i < ellnet::producer_count; i++)
        // {
        //     SPDLOG_INFO("new producer id {}", i);
        //     producers.emplace_back(ellnet::Producer, i);
        // }

        // for (int i = 0; i < ellnet::worker_count; i++)
        // {
        //     workers[i].join();
        // }

        // for (int i = 0; i < ellnet::producer_count; i++)
        // {
        //     producers[i].join();
        // }
        net_th.join();

        SPDLOG_INFO("shut down main thread");

        return 0;
    }
    catch (const std::exception &e)
    {
        SPDLOG_ERROR("caught a exception {}", e.what());
        ellnet::print_stacktrace();
    }
}