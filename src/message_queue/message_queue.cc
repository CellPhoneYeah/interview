#include "message_queue/message_queue.h"
#include "slog.h"

size_t MessageQueue::max_message_queue_size = 64;
// size_t MessageQueue::max_message_queue_size = 1 << 6;

int MessageQueue::PutM(const std::string &msg){
    std::unique_lock<std::shared_mutex> lock(mu_);
    if(messages_.size() >= max_message_queue_size){
        SPDLOG_WARN("mq size {} that is full!!", messages_.size());
        return -1;
    }
    messages_.push_back(std::move(msg));
    return 0;
}

std::string MessageQueue::PopM(){
    std::unique_lock<std::shared_mutex> lock(mu_);
    if(messages_.empty()){
        return "";
    }
    auto it = std::move(messages_.front());
    messages_.pop_front();
    return it;
}

std::string MessageQueue::GetTopM(){
    std::unique_lock<std::shared_mutex> lock(mu_);
    if(messages_.empty()){
        return "";
    }
    auto it = messages_.front();
    return it;
}

MessageQueue::MessageQueue(const int id)
{
    queue_id_ = id;
    messages_.clear();
}

MessageQueue::~MessageQueue()
{
    messages_.clear();
}