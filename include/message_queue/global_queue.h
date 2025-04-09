#ifndef GLOBAL_QUEUE_H
#define GLOBAL_QUEUE_H
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <deque>
#include <string>

#include "message_queue/message_queue.h"
#include "spdlog/slog.h"

class GlobalQueue{
public:
    static GlobalQueue* GetInstance();
    std::shared_ptr<MessageQueue> NewMQ(const int fd);
    std::shared_ptr<MessageQueue> NewMQWithoutCheck(const int fd);
    std::shared_ptr<MessageQueue> GetMQ(const int fd);
    std::shared_ptr<MessageQueue> PopMQ();
    std::shared_ptr<MessageQueue> PopMQHasMsg();
    void DelMQ(const int fd);
    void BackMQ(std::shared_ptr<MessageQueue> mq);
    int SendMsg(const int mqId, std::string msg);

    static size_t max_global_queue_size_;
    static GlobalQueue* instance_;
    static int max_queue_id_;
private:
    GlobalQueue();

    std::unordered_set<int> gq_has_msg_set_;
    std::deque<std::shared_ptr<MessageQueue>> gq_has_msg_;
    std::shared_mutex sh_mu_;
    std::mutex has_msg_mu_;
    std::unordered_map<int, std::shared_ptr<MessageQueue>> gq_map_;

};


#endif