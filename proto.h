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

struct Login{
    char name[32];
    char password[32];
};

struct LoginRet{
    int code;
};

struct Logout{
    char name[10];
};

struct LogoutRet{
    int code;
};

struct ChatMsg{
    char name[10];
    char msg[50];
};

struct ChatMsgRet{
    int code;
};

struct ErrorRet{
    int errorCode;
};

void handleLogin(int sock, struct DataHeader &dh){
    struct Login login;
    int len = recv(sock, &login, dh.dataLen, 0);
    if(len <= 0){
        std::cout << "login recv err" << std::endl;
    }else{
        std::cout << "login success " << login.name << std::endl;
        struct DataHeader dh;
        dh.cmd = LOGINRET;
        dh.dataLen = sizeof(LoginRet);
        struct LoginRet loginret;
        loginret.code = 0;
        send(sock, (const char *)&dh, sizeof(dh), 0);
        send(sock, (const char *)&loginret, dh.dataLen, 0);
    }
}

void handleLogout(int sock, struct DataHeader &dh){
    struct Logout logout;
    int len = recv(sock, &logout, dh.dataLen, 0);
    if(len <= 0){
        std::cout << "logout recv err" << std::endl;
    }else{
        std::cout << "logout success " << logout.name << std::endl;
        struct DataHeader dh;
        dh.cmd = LOGOUTRET;
        dh.dataLen = sizeof(LogoutRet);
        struct LogoutRet logoutret;
        logoutret.code = 0;
        send(sock, (const char*)&dh, sizeof(dh), 0);
        send(sock, (const char*)&logoutret, dh.dataLen, 0);
    }
}

void handleChat(int sock, struct DataHeader &dh){
    struct ChatMsg chatmsg;
    int len = recv(sock, &chatmsg, dh.dataLen, 0);
    if(len <= 0){
        std::cout << "chatmsg recv err" << std::endl;
    }else{
        std::cout << "chatmsg success " << chatmsg.name << std::endl;
        char data[sizeof(dh)];
        std::memcpy(data, &dh, sizeof(dh));
        doSendBrocastMessage(sock, data, sizeof(dh));
        char chatdata[dh.dataLen];
        std::memcpy(chatdata, &chatmsg, dh.dataLen);
        doSendBrocastMessage(sock, chatdata, dh.dataLen);

        struct DataHeader dh;
        dh.cmd = CHATRET;
        dh.dataLen = sizeof(ChatMsgRet);

        struct ChatMsgRet chatmsgret;
        chatmsgret.code = 0;
        send(sock, (const char*)&dh, sizeof(dh), 0);
        send(sock, (const char*)&chatmsgret, dh.dataLen, 0);
    }
}

int handleProto(int sock){
    struct DataHeader dh;
    int len = recv(sock, &dh, sizeof(dh), 0);
    if(len <= 0){
        std::cout << "recv err" << std::endl;
        return -1;
    }else{
        std::cout << "cmd:" << dh.cmd << std::endl;
        switch (dh.cmd)
        {
        case LOGIN:
            handleLogin(sock, dh);
            break;

        case LOGOUT:
            handleLogout(sock, dh);
            break;

        case CHAT:
            handleChat(sock, dh);
            break;
        
        default:
            std::cout << "server recv unexpected cmd:" << dh.cmd << " len:" << dh.dataLen << std::endl;
            break;
        }
        return 0;
    }
}

