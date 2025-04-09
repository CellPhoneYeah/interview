#include <unistd.h>

#include <thread>
#include <vector>
#include <ctime>
#include <chrono>

#include "message_queue/global_queue.h"
#include "ellnet/epoll_net.h"

namespace ellnet
{
    static std::string kListenHost = "127.0.0.1";
    static int kListenPort = 8088;
    int worker_count = 2;
    int producer_count = 1;
    int mq_count = 1;

    void NetWorker()
    {
        try{
            EpollNet * p_net = EpollNet::GetInstance();
            if(p_net->ListenOn(kListenHost, kListenPort) == 0){
                SPDLOG_INFO("start listen {}:{}", kListenHost, kListenPort);
            }else{
                SPDLOG_INFO("listen failed {}:{}", kListenHost, kListenPort);
            }
        }catch(const std::exception e){
            SPDLOG_ERROR("listen failed {}", e.what());
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

int main(){
    GlobalQueue* gq = GlobalQueue::GetInstance();
    for (int i = 0; i < ellnet::mq_count; i++)
    {
        SPDLOG_INFO("new mq id {}", i);
        gq->NewMQ(i);
    }
    
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
    //     workers[i].detach();
    // }

    // for (int i = 0; i < ellnet::producer_count; i++)
    // {
    //     producers[i].detach();
    // }

    std::thread net_th = std::thread(ellnet::NetWorker);
    net_th.detach();

    sleep(120);
    SPDLOG_INFO("shut down main thread");
    
    return 0;
}