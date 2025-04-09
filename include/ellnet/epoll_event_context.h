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
        EpollEventContext(const int fd, const bool listening);
        EpollEventContext();
        virtual ~EpollEventContext() override;

        virtual int HandleEvent(void *event) override;
        static void SetNoblocking(const int fd);
        void PushMsgQ(const char *msg, int size);
        void SetPipe() { pipe_flag_ = true; }
        bool IsPipe() { return pipe_flag_; }
        int BindMQ();
        inline int GetId(){return id_;}
    private:
        static int kMaxId;
        inline static int nextId() { return kMaxId ++;};
        inline void SetId(int id){id_ = id;};
        int HandleReadEvent(epoll_event *event);
        int HandleWriteEvent(epoll_event *event);
        void ProcessData(char *data, int size);
        void ModifyEv(int fd, int flag, bool enable);

        bool pipe_flag_ = false;
        int id_ = -1;
        std::shared_ptr<MessageQueue> mq_ = nullptr;
    };
}

#endif