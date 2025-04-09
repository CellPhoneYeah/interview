#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <deque>
#include <string>
#include <shared_mutex>
#include <mutex>

class MessageQueue
{
public:
    MessageQueue(const int id);
    ~MessageQueue();

    int GetId(){ return queue_id_;}
    bool HasMsg(){ return messages_.size() > 0;}
    int MsgSize(){ return messages_.size();}
    std::string PopM();
    std::string GetTopM();
    int PutM(const std::string& msg);

    static size_t max_message_queue_size;
private:
    int queue_id_;
    std::deque<std::string> messages_;
    std::shared_mutex mu_;
};

#endif