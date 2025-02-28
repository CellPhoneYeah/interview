#include "utility.h"
#include "EllBaseServer.h"
#include <thread>
#include <strings.h>
#include <iostream>
#include <unistd.h>
#include <cstring>

void runServer(bool &isRunning){
    EllBaseServer *ebs = new EllBaseServer();
    if(ebs->startListen(SERVER_HOST, SERVER_PORT) < 0){
        std::cout << "listen failed" << errno << std::endl;
        return;
    }
    int loopCount = 0;
    while(ebs->loopEVQ() >= 0 && isRunning){
        sleep(1);
        loopCount ++;
        if(loopCount % 20 == 0){
            loopCount = 0;
            // std::cout << "looping ..." << *ebs << std::endl;
        }
    }
    std::cout << "closing ..." << std::endl;
    delete(ebs);
}

int main()
{
    bool isRunning = true;
    std::thread th(runServer, std::ref(isRunning));
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