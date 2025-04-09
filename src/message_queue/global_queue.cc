#include "message_queue/global_queue.h"

size_t GlobalQueue::max_global_queue_size_ = 10240;

GlobalQueue *GlobalQueue::instance_ = nullptr;
int GlobalQueue::max_queue_id_ = 0;

int GlobalQueue::SendMsg(const int mqId, std::string msg){
    auto mq = GetMQ(mqId);
    if(mq != nullptr){
        if(mq->PutM(msg) < 0){
            SPDLOG_INFO("put m failed {}", mqId);
            return -1;
        }
        BackMQ(mq);
        return 0;
    }
    SPDLOG_INFO("mq not found {}", mqId);
    return -1;
}

void GlobalQueue::BackMQ(std::shared_ptr<MessageQueue> mq){
    std::lock_guard lock(has_msg_mu_);
    if(mq->HasMsg() && gq_has_msg_set_.find(mq->GetId()) == gq_has_msg_set_.end()){
        gq_has_msg_.push_back(mq);
        gq_has_msg_set_.insert(mq->GetId());
    }
}

std::shared_ptr<MessageQueue> GlobalQueue::PopMQHasMsg(){
    std::lock_guard lock(has_msg_mu_);
    if(gq_has_msg_set_.size() <= 0){
        // SPDLOG_INFO("not msg to handle");
        return nullptr;
    }
    auto it = gq_has_msg_.front();
    gq_has_msg_set_.erase(it->GetId());
    gq_has_msg_.pop_front();
    return it;
}

std::shared_ptr<MessageQueue> GlobalQueue::PopMQ(){
    std::lock_guard lock(has_msg_mu_);
    if(gq_has_msg_.size() <= 0){
        return nullptr;
    }
    auto it = gq_has_msg_.front();
    gq_has_msg_.pop_front();
    return it;
}

GlobalQueue* GlobalQueue::GetInstance(){
    if(instance_ == nullptr){
        instance_ = new GlobalQueue();
    }
    return instance_;
}

GlobalQueue::GlobalQueue(){
    gq_map_.clear();
    gq_has_msg_set_.clear();
    max_queue_id_ = 0;
}

std::shared_ptr<MessageQueue> GlobalQueue::GetMQ(const int id){
    std::shared_lock lock(sh_mu_);
    auto it = gq_map_.find(id);
    if(it != gq_map_.end()){
        return it->second;
    }
    return nullptr;
}

void GlobalQueue::DelMQ(const int id){
    std::shared_lock lock(sh_mu_);
    auto it = gq_map_.find(id);
    if(it != gq_map_.end()){
        gq_map_.erase(it);
        gq_has_msg_set_.erase(it->first);
    }
}

std::shared_ptr<MessageQueue> GlobalQueue::NewMQ(const int fd){
    std::shared_lock lock(sh_mu_);
    if(gq_map_.size() >= max_global_queue_size_){
        return nullptr;
    }
    std::shared_ptr<MessageQueue> new_q = std::make_shared<MessageQueue>(fd);
    gq_map_.insert(std::make_pair(fd, new_q));
    max_queue_id_++;
    SPDLOG_INFO("new mq {}", max_queue_id_);
    SPDLOG_INFO("new mq fd {} map size {}", fd, gq_map_.size());
    return new_q;
}

std::shared_ptr<MessageQueue> GlobalQueue::NewMQWithoutCheck(const int fd){
    std::shared_lock lock(sh_mu_);
    std::shared_ptr<MessageQueue> new_q = std::make_shared<MessageQueue>(fd);
    gq_map_.insert(std::make_pair(fd, new_q));
    max_queue_id_++;
    return new_q;
}