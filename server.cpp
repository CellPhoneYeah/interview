#include "utility.h"
#include "user.h"
#include "proto.h"
#include "EasyEllConn.h"

int main(){
    int kq = kqueue();
    if(kq < 0){
        std::cout << "run0";
        perror("kq create err");
        exit(-1);
    }
    EasyEllConn eecl(kq);
    if(!eecl.bindAddr(SERVER_HOST, SERVER_PORT)){
        std::cout << "run1";
        exit(-2);
    }
    if(!eecl.listen()){
        std::cout << "run2";
        exit(-3);
    }
    if(eecl.registerReadEv() < 0){
        std::cout << "run3";
        exit(-4);
    }
    struct kevent event_list[BUFFSIZE];
    cout << "start kqueue" << endl;
    while (true)
    {
        if(eecl.loopListenSock(event_list, BUFFSIZE) < 0){
            cout << "looping" << endl;
            break;
        }
    }
    std::cout << "runr";
    if(!eecl.isClosed())
        eecl.close();
    std::cout << "run6";
    close(kq);
    return 0;
}