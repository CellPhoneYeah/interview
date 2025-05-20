#ifndef EPOLL_NET_HEADER_H
#define EPOLL_NET_HEADER_H
namespace ellnet
{
    enum CommandType
    {
        CMD_INIT_LISTEN,
        CMD_INIT_CONNECT,
        CMD_CLOSE,
        CMD_START_LISTEN,
        CMD_START_CONNECT,
        CMD_EXIT,
        CMD_SEND_MSG,
    };

    enum SocketState
    {
        LISTEN_WAIT_OPEN,
        CONNECT_WAIT_OPEN,
        CONNECTED,
        LISTENING,
        WRITING_BEFORE_CLOSING,
        CLOSED,
        PIPE_LISTENING,
    };

    enum ManagerState
    {
        STOP,
        RUNNING
    };

    struct ControlCommand
    {
        CommandType cmd;
        int fd = -1;
        int sessionId = -1;
        int port = -1;
        char ipaddr[46]; // fix ipv4 ipv6
        int msgSize = -1; // size of msg to send
        char *msg = nullptr; // msg data to send
    };
} // namespace ellnet

#endif