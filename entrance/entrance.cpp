#include "slog.h"
#include "EpollNet.h"

int main(){
    EpollNet* net = EpollNet::getInstance();
    net->listenTo("127.0.0.1", 8088);
    return 0;
}