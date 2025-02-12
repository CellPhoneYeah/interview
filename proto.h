#include <sys/socket.h>
#include <iostream>
#include <string>
#include "utility.h"

enum PROTO{
    LOGIN,
    LOGINRET,
    LOGOUT,
    LOGOUTRET,
    CHAT,
    CHATRET,
    ERROR
};

struct DataHeader{
    short dataLen;
    short cmd;
};

#define DATAHEADER_LEN sizeof(DataHeader)

int recvWithoutHeader(int sock, char*byterecv, int len){
    return recv(sock, byterecv + DATAHEADER_LEN, len - DATAHEADER_LEN, 0);
}
int recvHeader(int sock, char*byterecv){
    return recv(sock, byterecv, DATAHEADER_LEN, 0);
}

struct Login: public DataHeader{
    Login(){
        cmd = LOGIN;
        dataLen = sizeof(Login);
    }
    char name[32];
    char password[32];
};

struct LoginRet:public DataHeader{
    LoginRet(){
        cmd = LOGINRET;
        dataLen = sizeof(LoginRet);
    }
    int code;
};

struct Logout: public DataHeader{
    Logout(){
        cmd = LOGOUT;
        dataLen = sizeof(Logout);
    }
    char name[10];
};

struct LogoutRet: public DataHeader{
    LogoutRet(){
        cmd = LOGOUTRET;
        dataLen = sizeof(LogoutRet);
    }
    int code;
};

struct ChatMsg: public DataHeader{
    ChatMsg(){
        cmd = CHAT;
        dataLen = sizeof(ChatMsg);
    }
    char name[10];
    char msg[50];
};

struct ChatMsgRet:public DataHeader{
    ChatMsgRet(){
        cmd = CHATRET;
        dataLen = sizeof(ChatMsgRet);
    }
    int code;
};

struct ErrorRet:public DataHeader{
    ErrorRet(){
        cmd = ERROR;
        dataLen = sizeof(ErrorRet);
    }
    int errorCode;
};

void handleLogin(int sock, char *byterecv, struct DataHeader *dh){
    int len = recvWithoutHeader(sock, byterecv, dh->dataLen);
    struct Login *login = (Login*)byterecv;
    if(len <= 0){
        std::cout << "login recv err" << std::endl;
    }else{
        std::cout << "login success " << login->name << std::endl;
        struct LoginRet loginret;
        loginret.code = 0;
        send(sock, (const char *)&loginret, loginret.dataLen, 0);
    }
}

void handleLogout(int sock, char *byterecv, struct DataHeader *dh){
    struct Logout logout;
    int len = recvWithoutHeader(sock, byterecv, dh->dataLen);
    if(len <= 0){
        std::cout << "logout recv err" << std::endl;
    }else{
        std::cout << "logout success " << logout.name << std::endl;
        struct LogoutRet logoutret;
        logoutret.code = 0;
        send(sock, (const char*)&logoutret, logoutret.dataLen, 0);
    }
}

void handleChat(int sock, char* byterecv, struct DataHeader *dh){
    int len = recvWithoutHeader(sock, byterecv, dh->dataLen);
    if(len <= 0){
        std::cout << "chatmsg recv err" << std::endl;
    }else{
        struct ChatMsg *chatmsg = (ChatMsg*)byterecv;
        std::cout << "chatmsg success " << chatmsg->name << std::endl;
        doSendBrocastMessage(sock, (char*)chatmsg, chatmsg->dataLen);

        struct ChatMsgRet chatmsgret;
        chatmsgret.code = 0;
        send(sock, (const char*)&chatmsgret, chatmsgret.dataLen, 0);
    }
}


int handleProto(int sock){
    char byteRecv[1024];
    int len = recvHeader(sock, byteRecv);
    if(len <= 0){
        std::cout << "recv err" << std::endl;
        return -1;
    }else{
        struct DataHeader *dh = (DataHeader*)byteRecv;
        std::cout << "cmd:" << dh->cmd << std::endl;
        switch (dh->cmd)
        {
        case LOGIN:
            handleLogin(sock, byteRecv, dh);
            break;

        case LOGOUT:
            handleLogout(sock, byteRecv, dh);
            break;

        case CHAT:
            handleChat(sock, byteRecv, dh);
            break;
        
        default:
            std::cout << "server recv unexpected cmd:" << dh->cmd << " len:" << dh->dataLen << std::endl;
            break;
        }
        return 0;
    }
}

