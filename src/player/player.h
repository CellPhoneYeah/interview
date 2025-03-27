//
// Created by yexiaofeng on 2025/2/26.
//

#ifndef PLAYER_H
#define PLAYER_H
class EllBaseServer;
class EasyEllConn;



class player {
private:
    EasyEllConn* pEec;
    EllBaseServer* pEbs;
    bool isRunning;
    int id;
public:
    player(EllBaseServer* pEbs, int id);
    ~player();
    void run();
    void stop();
};



#endif //PLAYER_H
