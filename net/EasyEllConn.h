#include "EllConn.h"
class EllBaseServer;

class EasyEllConn:public EllConn{
public:
    static void brocastAll(char* data, int size);
    EasyEllConn(const EllBaseServer* ebs): EllConn(ebs){}
    EasyEllConn(const EllBaseServer* ebs, int sockfd): EllConn(ebs, sockfd){
    }
    EasyEllConn(const EllBaseServer* ebs, int sockfd, int sockType): EllConn(ebs, sockfd, sockType){
    }

    void onCloseFd() override;

    bool acceptSock(int clientfd, EllConn* parentEC)override;

    int handleOneProto(const struct kevent &ev) override;
    ~EasyEllConn() override;
};