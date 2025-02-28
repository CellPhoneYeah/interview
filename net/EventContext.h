#include <queue>
#include <vector>

enum SOCKET_TYPE{
    SOCKET_TYPE_SOCK,
    SOCKET_TYPE_PIPE
};

struct EventContext{
    int fd = 0;
    std::queue<std::vector<char> > writeQ;
    std::vector<char> readBuffer;
    int offsetPos = 0;
    int socket_type = SOCKET_TYPE_SOCK;
    int targetfd;
};