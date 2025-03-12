#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H
#include <deque>
#include <string>
#include <shared_mutex>
class MessageQueue
{
private:
    int queueId;
    std::deque<std::string> messages;
    std::shared_mutex mu;
public:
    static size_t MAX_MEESSAGE_QUEUE_SIZE;
    int getId(){ return queueId;}
    bool hasMsg(){ return messages.size() > 0;}
    MessageQueue(int id);
    std::string popM();
    std::string getTopM();
    int putM(const std::string& msg);
    ~MessageQueue();
};

#endif

size_t MessageQueue::MAX_MEESSAGE_QUEUE_SIZE = 1024;

int MessageQueue::putM(const std::string &msg){
    std::shared_lock lock(mu);
    if(messages.size() >= MAX_MEESSAGE_QUEUE_SIZE){
        return -1;
    }
    messages.push_back(msg);
    return 0;
}

std::string MessageQueue::popM(){
    std::shared_lock lock(mu);
    auto it = messages.front();
    messages.pop_front();
    return it;
}

std::string MessageQueue::getTopM(){
    std::shared_lock lock(mu);
    auto it = messages.front();
    return it;
}

MessageQueue::MessageQueue(int id)
{
    queueId = id;
    messages.clear();
}

MessageQueue::~MessageQueue()
{
    messages.clear();
}
