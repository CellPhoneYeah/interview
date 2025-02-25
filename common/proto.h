#pragma once
#include <sys/socket.h>
#include <iostream>
#include <string>
#include "utility.h"

#define READ_BUFFER_SIZE 1024

enum PROTO{
    LOGIN,
    LOGINRET,
    LOGOUT,
    LOGOUTRET,
    CHAT,
    CHATRET,
    ERROR
};
// struct RingBuffer{
//     int head;
//     int tail;
//     char* buffer;
//     int size;
//     int maxSize;
//     RingBuffer(int maxSize){
//         if(maxSize > 20480){
//             exit(-2);
//         }
//         this->maxSize = maxSize;
//         this->buffer = new char[maxSize];
//     }

//     ~RingBuffer(){
//         delete[](this->buffer);
//     }

//     int writeBuffer(char *data, int len){
//         if(size + len >= maxSize){
//             return -1;
//         }
//         if(tail > head && tail + len > maxSize){
//             int firstPartSize = maxSize - tail;
//             memcpy(buffer + tail, data, firstPartSize);
//             memcpy(buffer, data + firstPartSize, len - firstPartSize);
//             size += len;
//             tail = len - firstPartSize;
//             return size;
//         }
//         memcpy(buffer + tail, data, len);
//         size += len;
//         tail += len;
//         return size;
//     }

//     int readBuffer(char* data, int len){
//         if(len <= 0){
//             return 0;
//         }
//         if(len > size){
//             if(head + size <= maxSize){
//                 memcpy(data, buffer + head, size);
//             }else{
//                 int firstPartSize = maxSize - head;
//                 memcpy(data, buffer + head, firstPartSize);
//                 memcpy(data + firstPartSize, buffer, size - firstPartSize);
//             }
//             return size;
//         }else{
//             if(head + len <= maxSize){
//                 memcpy(data, buffer + head, len);
//             }else{
//                 int firstPartSize = maxSize - head;
//                 memcpy(data, buffer + head, firstPartSize);
//                 memcpy(data + firstPartSize, buffer, len - firstPartSize);
//             }
//             return len;
//         }
//     }
// };

struct DataHeader{
    short dataLen;
    short cmd;
};

#define DATAHEADER_LEN sizeof(DataHeader)

// int recvWithoutHeader(int sock, char*byterecv, int len){
//     return recv(sock, byterecv + DATAHEADER_LEN, len - DATAHEADER_LEN, 0);
// }
// int recvHeader(int sock, char*byterecv){
//     return recv(sock, byterecv, DATAHEADER_LEN, 0);
// }

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

// void handleLogin(int sock, char *byterecv, struct DataHeader *dh){
//     int len = recvWithoutHeader(sock, byterecv, dh->dataLen);
//     struct Login *login = (Login*)byterecv;
//     if(len <= 0){
//         std::cout << "login recv err" << std::endl;
//     }else{
//         std::cout << "login success " << login->name << std::endl;
//         struct LoginRet loginret;
//         loginret.code = 0;
//         send(sock, (const char *)&loginret, loginret.dataLen, 0);
//     }
// }

// void handleLogout(int sock, char *byterecv, struct DataHeader *dh){
//     struct Logout logout;
//     int len = recvWithoutHeader(sock, byterecv, dh->dataLen);
//     if(len <= 0){
//         std::cout << "logout recv err" << std::endl;
//     }else{
//         std::cout << "logout success " << logout.name << std::endl;
//         struct LogoutRet logoutret;
//         logoutret.code = 0;
//         send(sock, (const char*)&logoutret, logoutret.dataLen, 0);
//     }
// }

// void handleChat(int sock, char* byterecv, struct DataHeader *dh){
//     int len = recvWithoutHeader(sock, byterecv, dh->dataLen);
//     if(len <= 0){
//         std::cout << "chatmsg recv err" << std::endl;
//     }else{
//         struct ChatMsg *chatmsg = (ChatMsg*)byterecv;
//         std::cout << "chatmsg success " << chatmsg->name << ":" << chatmsg->msg << std::endl;
//         // doSendBrocastMessage(sock, (char*)chatmsg, chatmsg->dataLen);

//         struct ChatMsgRet chatmsgret;
//         chatmsgret.code = 0;
//         send(sock, (const char*)&chatmsgret, chatmsgret.dataLen, 0);
//     }
// }

// void handleDataHeader(int sock, struct DataHeader* dh, char* byteRecv){
//     switch (dh->cmd)
//         {
//         case LOGIN:
//             handleLogin(sock, byteRecv, dh);
//             break;

//         case LOGOUT:
//             handleLogout(sock, byteRecv, dh);
//             break;

//         case CHAT:
//             handleChat(sock, byteRecv, dh);
//             break;
        
//         default:
//             std::cout << "server recv unexpected cmd:" << dh->cmd << " len:" << dh->dataLen << std::endl;
//             break;
//         }
// }

// bool readHeader(RingBuffer* rb, char*dataHeader){
//     return rb->readBuffer(dataHeader, DATAHEADER_LEN) == DATAHEADER_LEN;
// }

// int handleProto(int sock){
//     int totallen = 0;
//     int head = 0;
//     int tail = 0;
//     int cutedLen = 0;
//     char byteRecv[READ_BUFFER_SIZE];
//     RingBuffer rb(2048);
//     int len = recv(sock, byteRecv, READ_BUFFER_SIZE, 0);
//     char *dataHeader;
//     DataHeader* dh = nullptr;
//     bool headerReaded = false;
//     while(len > 0){
//         if(rb.size + len <= rb.maxSize){
//             rb.writeBuffer(byteRecv, len);
//         }else{
//             int tmpSize = rb.maxSize - rb.size;
//             rb.writeBuffer(byteRecv, tmpSize);
//             len -= tmpSize;
//         }
//         if(rb.size >= sizeof(DataHeader) && !headerReaded){
//             if(readHeader(&rb, dataHeader)){
//                 dh = (DataHeader*)dataHeader;
//             }else{
//                 std::cout << "read header failed" << std::endl;
//             }
//         }else if(headerReaded && rb.size >= dh->dataLen){
//         }
//     }
//     if(len <= 0){
//         std::cout << "clent closed " << sock << std::endl;
//         return -1;
//     }else{
//         struct DataHeader *dh = (DataHeader*)byteRecv;
//         // std::cout << "cmd:" << dh->cmd << std::endl;
        
//         return 0;
//     }
// }

