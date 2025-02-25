#include "EllConn.h"
#include <iostream>

class EasyEllConn:public EllConn{
public:
    static void brocastAll(char* data, int size);
    EasyEllConn(int kq): EllConn(kq){}
    EasyEllConn(int kq, int sockfd): EllConn(kq, sockfd){
    }
    EasyEllConn(int kq, int sockfd, int sockType): EllConn(kq, sockfd, sockType){
    }

    void onCloseFd() override;

    bool acceptSock(int clientfd, EllConn* parentEC)override;

    int handleOneProto(const struct kevent &ev) override;
    ~EasyEllConn() override;
};