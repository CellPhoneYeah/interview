#include <iostream>
#include "EpollManager.h"
#include <string.h>

bool isRun = false;

void RunClient(){
    EpollManager *emgr = new EpollManager();
    emgr->connect_to("127.0.0.1", 8088);
    isRun = true;
    while(1){
        if(isRun){
            emgr->loop();
        }
        else
        {
            std::cout << "stop client \n";
            break;
        }
    }
}

int main(){
    while(1){
        char str[1024];
        fgets(str, 1024, stdin);
        if(strcmp(str, "stop") == 0){
            isRun = false;
            break;
        }
        isRun = true;
    }
    return 0;
}