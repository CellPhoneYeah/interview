#include "event_context.h"
#include <string.h>
#include <iostream>
#include "epoll_manager.h"
#include "slog.h"

namespace ellnet
{
    EventContext::EventContext(const int fd)
    {
        SetFd(fd);
        read_buffer_.resize(EpollManager::kMaxEpollReadSize);
    }

    EventContext::EventContext(const int fd, const bool is_listen)
    {
        SetFd(fd);
        opening_ = false;
        listening_ = is_listen;
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
        // std::cout << std::string(readBuffer.data()) << std::endl;
        SPDLOG_INFO("recv msg from {}", ownfd_);
        Living();
        read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + offset_pos_);
        offset_pos_ = 0;
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
