
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