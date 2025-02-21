#include "EllConn.h"
class EasyEllConn:public EllConn{
public:
    static void brocastAll(char* data, int size);
    EasyEllConn(int kq): EllConn(kq){}
    EasyEllConn(int kq, int sockfd): EllConn(kq, sockfd){
    }

    void onCloseFd() override;

    bool acceptSock(int clientfd, EllConn* parentEC)override;

    int handleOneProto() override;
    ~EasyEllConn() override;
};