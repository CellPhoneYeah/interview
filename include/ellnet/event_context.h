#ifndef EVENT_CONTEXT_H
#define EVENT_CONTEXT_H
#include <queue>
#include <vector>
#include <string>
#include <atomic>

#include "epoll_net_header.h"
namespace ellnet
{

#define MAX_READ_BUFFER_SIZE 1024

    enum HandleEventCode
    {
        HANDLE_EVENT_CODE_OK,
        HANDLE_EVENT_CODE_CONNECT_RESET,
        HANDLE_EVENT_CODE_ACCEPT_ET_WAIT,
        HANDLE_EVENT_CODE_ACCEPT_FAILED,
        HANDLE_EVENT_CODE_ACCEPT_SIGNAL_BREAK,
        HANDLE_EVENT_CODE_ACCEPT_REMOTE_CLOSED,
        HANDLE_EVENT_CODE_REMOTE_CONNECT_CLOSED,
        HANDLE_EVENT_CODE_UNKNOW_ERR,
        HANDLE_EVENT_CODE_SEND_QUEUE_EMPTY,
    };

    class EventContext
    {
    public:
        std::queue<std::vector<char>> send_q_;
        std::vector<char> read_buffer_;
        std::vector<char> tmp_buffer_;
        int offset_pos_ = 0;
        int read_offset_ = 0;

        explicit EventContext(const int fd);
        explicit EventContext(const int fd, const SocketState initState);
        EventContext(const int fd, const bool is_listen);
        void ReadBytes(const int byte_len);
        int GetFd() { return ownfd_; }
        bool IsListening() { return this->state == SocketState::LISTENING; }
        bool IsClosed() { return this->state == SocketState::CLOSED; }
        bool IsConnecting() { return connecting_; }
        void ClearSendQ();
        void ConnectStart() { connecting_ = true; }
        void Dead() { is_living_ = false; }
        void Living() { is_living_ = true; }
        bool IsDead() { return !is_living_ && !opening_; }
        void SetAddrPort(std::string &ipaddr, const int port);
        bool IsListening(std::string &ipaddr, const int port);
        std::string GetIpAddr(){return std::string(ipaddr_);}
        int GetPort(){return port_;}
        void finishOpen(){opening_ = false;}
        void SetFd(int fd);
        void SetState(SocketState state){this->state = state;}
        SocketState GetState(){return this->state;}

        int GetSessionId(){return this->session_id_;}
        void SetSessionId(int id){this->session_id_ = id;}
        int NextSessionId(){return ++this->kMaxSessionId;}

        virtual ~EventContext() = 0;
        virtual int HandleEvent(void *event) = 0;

    protected:
        static std::atomic<int> kMaxSessionId;
        int session_id_ = 0;
        int ownfd_ = 0;
        int socket_type_ = 0;
        bool listening_ = false;
        bool connecting_ = false;
        bool opening_ = false;
        bool is_living_ = false;
        char ipaddr_[46];
        int port_;
        SocketState state;
    };
}

#endif