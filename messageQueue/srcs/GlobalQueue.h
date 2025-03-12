#ifndef GLOBAL_QUEUE_H
#define GLOBAL_QUEUE_H
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include "MessageQueue.h"
#include <memory>
#include <deque>
#include <string>

class GlobalQueue{
private:
    GlobalQueue();
    std::unordered_set<int> gqHasMsgSet;
    std::deque<std::shared_ptr<MessageQueue>> gqHasMsg;

    std::shared_mutex shMu;
    std::mutex hasMsgMu;
    std::unordered_map<int, std::shared_ptr<MessageQueue>> gqMap;
public:
    static size_t MAX_GLOBAL_QUEUE_SIZE;
    static GlobalQueue* getInstance();
    static GlobalQueue* instance;
    std::shared_ptr<MessageQueue> newMQ();
    std::shared_ptr<MessageQueue> newMQWithoutCheck();
    void delMQ(int id);
    std::shared_ptr<MessageQueue> getMQ(int id);
    std::shared_ptr<MessageQueue> popMQ();
    std::shared_ptr<MessageQueue> popMQHasMsg();
    void backMQ(std::shared_ptr<MessageQueue> mq);
    int sendMsg(int mqId, std::string msg);
    static int maxQueueId;
};

#endif
size_t GlobalQueue::MAX_GLOBAL_QUEUE_SIZE = 10240;

GlobalQueue* GlobalQueue::instance = nullptr;
int GlobalQueue::maxQueueId = 0;

int GlobalQueue::sendMsg(int mqId, std::string msg){
    auto mq = getMQ(mqId);
    if(mq != nullptr){
        mq->putM(msg);
        backMQ(mq);
    }
    return 0;
}

void GlobalQueue::backMQ(std::shared_ptr<MessageQueue> mq){
    std::lock_guard lock(hasMsgMu);
    if(mq->hasMsg() && gqHasMsgSet.find(mq->getId()) == gqHasMsgSet.end()){
        gqHasMsg.push_back(mq);
        gqHasMsgSet.insert(mq->getId());
    }
}

std::shared_ptr<MessageQueue> GlobalQueue::popMQHasMsg(){
    std::lock_guard lock(hasMsgMu);
    if(gqHasMsgSet.size() <= 0){
        // SPDLOG_INFO("not msg to handle");
        return nullptr;
    }
    auto it = gqHasMsg.front();
    gqHasMsgSet.erase(it->getId());
    gqHasMsg.pop_front();
    return it;
}

std::shared_ptr<MessageQueue> GlobalQueue::popMQ(){
    std::lock_guard lock(hasMsgMu);
    if(gqHasMsg.size() <= 0){
        return nullptr;
    }
    auto it = gqHasMsg.front();
    gqHasMsg.pop_front();
    return it;
}

GlobalQueue* GlobalQueue::getInstance(){
    if(instance == nullptr){
        instance = new GlobalQueue();
    }
    return instance;
}

GlobalQueue::GlobalQueue(){
    gqMap.clear();
    gqHasMsgSet.clear();
    maxQueueId = 0;
}

std::shared_ptr<MessageQueue> GlobalQueue::getMQ(int id){
    std::shared_lock lock(shMu);
    auto it = gqMap.find(id);
    if(it != gqMap.end()){
        return it->second;
    }
    return nullptr;
}

void GlobalQueue::delMQ(int id){
    std::shared_lock lock(shMu);
    auto it = gqMap.find(id);
    if(it != gqMap.end()){
        gqMap.erase(it);
        gqHasMsgSet.erase(it->first);
    }
}

std::shared_ptr<MessageQueue> GlobalQueue::newMQ(){
    std::shared_lock lock(shMu);
    if(gqMap.size() >= MAX_GLOBAL_QUEUE_SIZE){
        return nullptr;
    }
    std::shared_ptr<MessageQueue> newQ = std::make_shared<MessageQueue>(++maxQueueId);
    gqMap.insert(std::make_pair(maxQueueId, newQ));
    SPDLOG_INFO("new mq {}", maxQueueId);
    return newQ;
}

std::shared_ptr<MessageQueue> GlobalQueue::newMQWithoutCheck(){
    std::shared_lock lock(shMu);
    std::shared_ptr<MessageQueue> newQ = std::make_shared<MessageQueue>(++maxQueueId);
    gqMap.insert(std::make_pair(maxQueueId, newQ));
    return newQ;
}
