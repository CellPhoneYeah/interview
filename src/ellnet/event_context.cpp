#include "event_context.h"
#include <string.h>
#include <iostream>
#include "epoll_manager.h"
#include "slog.h"

namespace ellnet
{
    std::atomic<int> EventContext::kMaxSessionId = 0;
    std::mutex kSessionMutex;

    EventContext::EventContext(const int fd):EventContext(fd, SocketState::CLOSED)
    {
    }

    EventContext::EventContext(const int fd, const SocketState initState)
    {
        SetFd(fd);
        SetState(initState);
        SetSessionId(EventContext::NextSessionId());
        read_buffer_.resize(EpollManager::kMaxEpollReadSize);
        tmp_buffer_.resize(1024);
        SPDLOG_INFO("do create EventContext session {} fd {} state {}", session_id_, fd, (int)initState);
    }

    EventContext::EventContext(const int fd, const bool is_listen)
    {
        SetFd(fd);
        opening_ = false;
        listening_ = is_listen;
        if(is_listen){
            SetState(CLOSED);
        }else{
            SetState(LISTEN_WAIT_OPEN);
        }
        tmp_buffer_.resize(1024);
        read_buffer_.resize(EpollManager::kMaxEpollEventNum);
    }

    void EventContext::ClearSendQ()
    {
        std::queue<std::vector<char>>().swap(send_q_);
        offset_pos_ = 0;
    }

    void EventContext::SetAddrPort(std::string &ipaddr, const int port)
    {
        listening_ = true;
        strcpy(this->ipaddr_, ipaddr.c_str());
        this->port_ = port;
    }

    bool EventContext::IsListening(std::string &ipaddr, const int port)
    {
        if (!listening_)
        {
            return false;
        }
        return strcmp(this->ipaddr_, ipaddr.c_str()) && this->port_ == port;
    }

    void EventContext::ReadBytes(const int byte_len)
    {
        if(byte_len < 0){
            SPDLOG_ERROR("read byte len illegal {}", byte_len);
            return;
        }
        if(read_offset_ > read_buffer_.size()){
            SPDLOG_ERROR("read buffer offset pos illegal offset {} size {}", read_offset_, read_buffer_.size());
            return;
        }
        SPDLOG_INFO(" recv msg from fd:{} size:{} offset:{}", ownfd_, byte_len, read_offset_);
        Living();
        std::string msg(read_buffer_.begin(), read_buffer_.begin() + byte_len);
        read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + byte_len);
        SPDLOG_INFO("after recv msg from {} msg:{} size:{}", ownfd_, msg, msg.size());
        read_offset_ = 0;
    }

    EventContext::~EventContext()
    {
        // std::cout << "release EventContext" << ownfd << std::endl;
    }

    void EventContext::SetFd(int fd)
    {
        opening_ = false;
        ownfd_ = fd;
    }
}
