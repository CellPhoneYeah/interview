#include "EllBaseServer.h"

bool EllBaseServer::start(string ipaddr, int port){
    eec.bindAddr(ipaddr, port);
}