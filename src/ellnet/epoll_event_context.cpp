#include "epoll_event_context.h"

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>

#include "message_queue/global_queue.h"

namespace ellnet
{
    int EpollEventContext::kMaxId = 0;

    int EpollEventContext::HandleEvent(void *event)
    {
        struct epoll_event *ev = (struct epoll_event *)event;
        if (ev->events & EPOLLIN)
        {
            return HandleReadEvent(ev);
        }
        if (ev->events & EPOLLOUT)
        {
            return HandleWriteEvent(ev);
        }
        return -1;
    }

    int EpollEventContext::HandleReadEvent(epoll_event *event)
    {
        if (IsListening())
        {
            struct sockaddr_in addr;
            socklen_t len = sizeof(addr);
            while (1)
            {
                int newFd = accept(ownfd_, (sockaddr *)&addr, &len);
                if (newFd < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // 边缘模式下，暂时没有事件,等待下一次事件到来
                        return HANDLE_EVENT_CODE_OK;
                    }
                    return HANDLE_EVENT_CODE_ACCEPT_FAILED;
                }
                EpollEventContext *newContext = new EpollEventContext(newFd);
                newContext->SetNoblocking();
                int flag = EPOLLOUT | EPOLLET; // 边缘触发
                event->events = flag;
                event->data.ptr = newContext;
                epoll_ctl(ownfd_, EPOLL_CTL_ADD, newFd, event);
            }
            return HANDLE_EVENT_CODE_OK;
        }

        read_buffer_.resize(1024);
        int readbytes = recv(ownfd_, read_buffer_.data(), read_buffer_.size(), MSG_NOSIGNAL);
        if (readbytes > 0)
        {
            ProcessData(read_buffer_.data(), readbytes);
        }
        if (readbytes == 0)
        {
            // 已经断开连接
            return HANDLE_EVENT_CODE_REMOTE_CONNECT_CLOSED;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 边缘触发时，属于正常读取结束
            return HANDLE_EVENT_CODE_OK;
        }
        if (errno == ECONNRESET)
        {
            return HANDLE_EVENT_CODE_CONNECT_RESET;
        }
        std::cout << "recv failed for unknow err" << errno << std::endl;
        return HANDLE_EVENT_CODE_UNKNOW_ERR;
    }

    int EpollEventContext::HandleWriteEvent(epoll_event *event)
    {
        if (send_q_.size() == 0)
        {
            return HANDLE_EVENT_CODE_SEND_QUEUE_EMPTY;
        }
        return 0;
    }

    void EpollEventContext::SetNoblocking()
    {
        int flag = fcntl(ownfd_, F_GETFL, 0);
        fcntl(ownfd_, F_SETFL, flag | O_NONBLOCK);
    }

    void EpollEventContext::SetBlocking()
    {
        int flag = fcntl(ownfd_, F_GETFL, 0);
        fcntl(ownfd_, F_SETFL, flag & ~O_NONBLOCK);
    }

    void EpollEventContext::PushMsgQ(const char *msg, const int size)
    {
        if (send_q_.size() >= 1024)
        {
            std::cout << "send queue full " << ownfd_ << std::endl;
            return;
        }
        send_q_.push(std::vector<char>(msg, msg + size));
        std::cout << send_q_.size() << " insert send queue success " << ownfd_ << " : " << std::string(msg) << std::endl;
        delete msg;
    }

    void EpollEventContext::ProcessData(char *data, int size)
    {
        std::cout << "process data" << std::string(data) << std::endl;
    }

    void EpollEventContext::ModifyEv(const int fd, const int flag, const bool enable)
    {
        int current_flag = fcntl(fd, F_GETFL, 0);
        if (enable)
        {
            current_flag &= flag;
        }
        else
        {
            current_flag &= ~flag;
        }
        fcntl(fd, F_SETFL, current_flag);
    }

    EpollEventContext::EpollEventContext() : EventContext(-1)
    {
        connecting_ = false;
        opening_ = true;
    }

    EpollEventContext::EpollEventContext(const int fd) : EventContext(fd)
    {
        connecting_ = false;
    }

    EpollEventContext::EpollEventContext(const int fd, const SocketState initState) : EventContext(fd, initState)
    {
        connecting_ = false;
    }

    EpollEventContext::EpollEventContext(const int fd, bool listening) : EventContext(fd, listening)
    {
        connecting_ = false;
    }


    EpollEventContext::~EpollEventContext()
    {
        mq_ = nullptr;
        // std::cout << "release EpollEventContext :" << ownfd << std::endl;
    }

    int EpollEventContext::BindMQ(){
        GlobalQueue *const gq = GlobalQueue::GetInstance();
        if(gq->GetMQ(ownfd_) == nullptr){
            std::shared_ptr<MessageQueue> newmq = gq->NewMQ(ownfd_);
            mq_ = newmq;
        }
        return 0;
    }
}