#include <iostream>
#include <unistd.h>
#include "proto.h"

#define READED_BUFFER_SIZE 10240
class netclient{
private:
    int _sock = -1;
    char _readed_buffer[READED_BUFFER_SIZE];
    int _last_pos = 0;
    int _size = 0;
    DataHeader* dh = nullptr;
public:
    netclient(int sock){
        _sock = sock;
        _last_pos = 0;
        bzero(_readed_buffer, READED_BUFFER_SIZE);
        dh = nullptr;
    }

    ~netclient(){
        if(_sock != -1){
            close();
        }
        std::cout << "net closed" << std::endl;
    }

    int close(){
        if(_sock != -1)
            ::close(_sock);
        _sock = -1;
    }

    int recvData(){
        int len = recv(_sock, _readed_buffer, READED_BUFFER_SIZE, 0);
        _size += len;
        while(_size >= DATAHEADER_LEN){
            dh = (DataHeader*)(_readed_buffer + _last_pos);
            if(_size - DATAHEADER_LEN >= dh->dataLen){
                readOneProto();
            }else{
                std::cout << "size not enough for one proto" << std::endl;
            }
        }
    }

    int readOneProto()
    {
        std::cout << "recv cmd:" << dh->cmd << std::endl;
        switch (dh->cmd)
        {
        case LOGIN:
        {
            Login *login = (Login*)(_readed_buffer + _last_pos);
            break;
        }
        case LOGINRET:
        {
            LoginRet *loginret = (LoginRet*)(_readed_buffer + _last_pos);
            break;
        }
        case LOGOUT:
        {
            Logout *logout = (Logout*)(_readed_buffer + _last_pos);
            break;
        }
        case LOGOUTRET:
        {
            LogoutRet *logoutret = (LogoutRet*)(_readed_buffer + _last_pos);
            break;
        }
        case CHAT:
        {
            ChatMsg*chatmsg = (ChatMsg*)(_readed_buffer + _last_pos);
            break;
        }
        case CHATRET:
        {
            ChatMsgRet*chatmsgret = (ChatMsgRet*)(_readed_buffer + _last_pos);
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
        _last_pos += dh->dataLen;
    }
};