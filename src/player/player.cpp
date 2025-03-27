//
// Created by yexiaofeng on 2025/2/26.
//

#include "player.h"
#include "EllBaseServer.h"

player::player(EllBaseServer* pEbs, int id) {
    pEec = new EasyEllConn(pEbs);
    this->pEbs = pEbs;
    isRunning = false;
    this->id = id;
}

void player::run() {
    if(pEec->isClosed()) {
        if(pEec->connect(pEbs->getListenedAddr().c_str(), pEbs->getListenedPort()) < 0) {
            std::cout << "connect err:" << pEec->getSock() << std::endl;
            return;
        }
        char buffer[1024];
        std::string str = "hello I'm " + id;
        while (isRunning) {
            strcpy(buffer, str.c_str());
            pEec->sendData(buffer, 1024);
            sleep(5);
        }
    }
}

void player::stop() {
    pEec->close();
}

player::~player() {
    if(!pEec->isClosed()) {
        pEec->close();
    }
    delete(pEec);
    isRunning = false;
}



