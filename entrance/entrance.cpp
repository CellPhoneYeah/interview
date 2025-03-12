#include "EpollNet.h"
#include "watchdog.h"
#include "WatchDogConnectHandler.h"
#include <thread>
#include <vector>
#include <ctime>
#include "GlobalQueue.h"

int workerCount = 2;
int producerCount = 1;
int MQCount = 1;

void Worker(int id){
    SPDLOG_INFO("start worker {}", id);
    GlobalQueue* gq = GlobalQueue::getInstance();
    std::shared_ptr<MessageQueue> mq;
    int handleCount = 0;
    for(;;){
        mq = gq->popMQHasMsg();
        if(mq != nullptr){
            std::string msg = mq->popM();
            handleCount ++;
            SPDLOG_INFO("worker :{} count:{} handle mq:{} msg:{}", id, handleCount, mq->getId(), msg);
            gq->backMQ(mq);
        }else{
            // SPDLOG_INFO("worker :{} do nothing", id);
            sleep(1);
        }
    }
}

void Producer(int producerId){
    SPDLOG_INFO("start producer {}", producerId);
    GlobalQueue* gq = GlobalQueue::getInstance();
    std::shared_ptr<MessageQueue> mq;
    int sendCount = 0;
    for(;;){
        std::srand(std::time(nullptr));
        int randomId = std::rand() % MQCount + 1;
        std::string msg = "msg from producer" + std::to_string(producerId);
        if(gq->sendMsg(randomId, msg) < 0){
            SPDLOG_INFO("producer :{} try send to {} failed", producerId, randomId);
            sleep(1);
        }else{
            sendCount ++;
            SPDLOG_INFO("producer {} count:{} send msg to {} ", producerId, sendCount, randomId);
            sleep(1);
        }
    }
}

int main(){
    watchdog *wd = new watchdog(1);
    watchdog::putDog(wd);
    EpollNet *eNet = EpollNet::getInstance();
    eNet->setConnectHandler(new WatchDogConnectHandler());
    eNet->listenOn("127.0.0.1", 8088);

    GlobalQueue* gq = GlobalQueue::getInstance();
    for (int i = 0; i < MQCount; i++)
    {
        gq->newMQ();
    }
    
    std::vector<std::thread> workers;
    std::vector<std::thread> producers;
    for (int i = 0; i < workerCount; i++)
    {
        workers.emplace_back(Worker, i);
    }

    for (int i = 0; i < producerCount; i++)
    {
        producers.emplace_back(Producer, i);
    }

    for (int i = 0; i < workerCount; i++)
    {
        workers[i].detach();
    }

    for (int i = 0; i < producerCount; i++)
    {
        producers[i].detach();
    }

    sleep(600);
    SPDLOG_INFO("shut down main thread");
    
    return 0;
}