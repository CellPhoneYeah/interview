#pragma once
#include <iostream>
#include <unistd.h>
#include "proto.h"

#define RING_BUFFER_SIZE 10240
#define READ_BUFFER_SIZE 1024
class netclient{
private:
    int _sockfd;
    char _ring_buffer[RING_BUFFER_SIZE];
    char _read_buffer[READ_BUFFER_SIZE];
    int _read_pos;
    int _last_pos;
    int _size;
    DataHeader* dh;
public:
    netclient(int sock){
        _sockfd = sock;
        _last_pos = 0;
        bzero(_ring_buffer, RING_BUFFER_SIZE);
        bzero(_read_buffer, READ_BUFFER_SIZE);
        dh = nullptr;
        _read_pos = 0;
    }

    ~netclient(){
        if(_sockfd != -1){
            close();
        }
        std::cout << "net closed" << std::endl;
    }

    void close(){
        if(_sockfd != -1)
            ::close(_sockfd);
        _sockfd = -1;
    }

    int getSock(){
        return _sockfd;
    }

    int recvData(){
        int len;
        if(_size == 0){
            len = recv(_sockfd, _ring_buffer, RING_BUFFER_SIZE, 0);
        }else{
            len = recv(_sockfd, _ring_buffer + _size, RING_BUFFER_SIZE - _size, 0);
        }
        if(len <= 0){
            return len;
        }
        _size += len;
        int leftLen = len;
        int maxloop = 1000;
        int loopCount = 0;
        while(leftLen > 0){
            if(dh == nullptr){
                if (leftLen >= DATAHEADER_LEN)
                {
                    memcpy(_read_buffer, _ring_buffer + _last_pos, DATAHEADER_LEN);
                    dh = (DataHeader *)(_read_buffer + _read_pos);
                    _last_pos += DATAHEADER_LEN;
                    _size -= DATAHEADER_LEN;
                    leftLen -= DATAHEADER_LEN;
                    _read_pos = DATAHEADER_LEN;
                }else{
                    memcpy(_read_buffer + _read_pos, _ring_buffer + _last_pos, leftLen);
                    _last_pos += leftLen;
                    leftLen = 0;
                    _read_pos += leftLen;
                    _size = 0;
                }
            }else{
                int needReadSize = dh->dataLen - DATAHEADER_LEN;
                if(leftLen >= needReadSize){
                    memcpy(_read_buffer + _read_pos, _ring_buffer + _last_pos, needReadSize);
                    readOneProto();
                    _last_pos += needReadSize;
                    leftLen -= needReadSize;
                    _size -= needReadSize;
                    _read_pos = 0;
                    dh = nullptr;
                }else{
                    memcpy(_read_buffer + _read_pos, _ring_buffer + _last_pos, leftLen);
                    _last_pos += leftLen;
                    _read_pos += leftLen;
                    leftLen = 0;
                    _size = 0;
                    
                    std::cout << "not enough one proto size" << std::endl;
                }
            }
            loopCount++;
            if(loopCount >= maxloop){
                return -1;
            }
        }
        _last_pos = 0;
        return len;
    }

    int readOneProto()
    {
        std::cout << "recv cmd:" << dh->cmd << std::endl;
        switch (dh->cmd)
        {
        case LOGIN:
        {
            Login *login = (Login*)(_read_buffer);
            std::cout << "login " << login->name << " :" << login->password;
            break;
        }
        case LOGINRET:
        {
            LoginRet *loginret = (LoginRet*)(_read_buffer);
            std::cout << "loginret " << loginret->code;
            break;
        }
        case LOGOUT:
        {
            Logout *logout = (Logout*)(_read_buffer);
            std::cout << "logout " << logout->name;
            break;
        }
        case LOGOUTRET:
        {
            LogoutRet *logoutret = (LogoutRet*)(_read_buffer);
            std::cout << "logoutret " << logoutret->code;
            break;
        }
        case CHAT:
        {
            ChatMsg*chatmsg = (ChatMsg*)(_read_buffer);
            std::cout << "chatmsg " << chatmsg->name << ":" << chatmsg->msg << std::endl;
            break;
        }
        case CHATRET:
        {
            ChatMsgRet*chatmsgret = (ChatMsgRet*)(_read_buffer);
            std::cout << "chatmsgret " << chatmsgret->code;
            break;
        }
        case ERROR:
        {
            break;
        }
        
        default:
            std::cout << "unexpected proto type:" << dh->cmd << std::endl;
            break;
        }
        return 0;
    }
};