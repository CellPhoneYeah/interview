#ifndef EPOLL_EVENT_CONTEXT_H
#define EPOLL_EVENT_CONTEXT_H
#include "event_context.h"

#include <sys/epoll.h>
#include <memory>

#include "message_queue/message_queue.h"

namespace ellnet
{
    class EpollEventContext : public EventContext
    {
    public:
        explicit EpollEventContext(const int fd);
        explicit EpollEventContext(const int fd, const SocketState initState);
        EpollEventContext(const int fd, const bool listening);
        EpollEventContext();
        virtual ~EpollEventContext() override;

        virtual int HandleEvent(void *event) override;
        void SetNoblocking();
        void SetBlocking();
        void PushMsgQ(const char *msg, int size);
        void SetPipe() { pipe_flag_ = true; }
        bool IsPipe() { return pipe_flag_; }
        int BindMQ();
    private:
        static int kMaxId;
        int HandleReadEvent(epoll_event *event);
        int HandleWriteEvent(epoll_event *event);
        void ProcessData(char *data, int size);
        void ModifyEv(int fd, int flag, bool enable);

        bool pipe_flag_ = false;
        std::shared_ptr<MessageQueue> mq_ = nullptr;
    };
}

#endif // EPOLL_EVENT_CONTEXT_H