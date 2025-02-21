#include "EasyEllConn.h"
#include 

class EllBaseServer{
private:
    EasyEllConn eec;

public:
    EllBaseServer(){
    }
    bool start(string ipaddr, int port);
};