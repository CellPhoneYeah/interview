#include "utility.h"
#include "user.h"
#include "proto.h"
#include "EllBaseServer.h"
#include <thread>

void runServer(bool &isRunning){
    // int kq = kqueue();
    // if(kq < 0){
    //     std::cout << "run0";
    //     perror("kq create err");
    //     exit(-1);
    // }
    // EasyEllConn eecl(kq);
    // if(!eecl.bindAddr(SERVER_HOST, SERVER_PORT)){
    //     std::cout << "run1";
    //     exit(-2);
    // }
    // if(!eecl.listen()){
    //     std::cout << "run2";
    //     exit(-3);
    // }
    // struct kevent event_list[BUFFSIZE];
    // cout << "start kqueue" << endl;
    // while (isRunning)
    // {
    //     if(eecl.loopListenSock(event_list, BUFFSIZE) < 0){
    //         cout << "looping" << endl;
    //         break;
    //     }
    // }
    // std::cout << "runr";
    // if(!eecl.isClosed())
    //     eecl.close();
    // std::cout << "run6";
    // close(kq);
    EllBaseServer *ebs = new EllBaseServer();
    if(ebs->startListen(SERVER_HOST, SERVER_PORT) < 0){
        std::cout << "listen failed" << errno << std::endl;
        return;
    }
    int loopCount = 0;
    while(ebs->loopKQ() >= 0 && isRunning){
        sleep(1);
        loopCount ++;
        if(loopCount % 20 == 0){
            loopCount = 0;
            std::cout << "looping ..." << *ebs << std::endl;
        }
    }
    std::cout << "closing ..." << std::endl;
    delete(ebs);
}

int main()
{
    bool isRunning = true;
    thread th(runServer, std::ref(isRunning));
    th.detach();
    while (1)
    {
        char w[20];
        fgets(w, 20, stdin);
        if (strncasecmp(w, EXITSTR, strlen(EXITSTR)) == 0)
        {
            isRunning = false;
        }
        else if (strncasecmp(w, STOPSTR, strlen(STOPSTR)) == 0)
        {
            break;
        }
        else
        {
            std::cout << "input:" << w << std::endl;
        }
    }

    return 0;
}