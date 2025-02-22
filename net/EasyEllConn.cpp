#include "EasyEllConn.h"

void EasyEllConn::onCloseFd()
{
}

bool EasyEllConn::acceptSock(int clientfd, EllConn *parentEC)
{
    if (clientfd == _sockfd)
    {
        return true;
    }
    if (EllConn::getClient(clientfd) == nullptr)
    {
        EasyEllConn *eec = new EasyEllConn(parentEC->getKQ(), clientfd);
        eec->registerReadEv();
        EllConn::addClient(clientfd, eec);
        return true;
    }
    else
    {
        std::cout << " repeat sock data, check mem leek" << std::endl;
        return false;
    }
}

int EasyEllConn::handleOneProto()
{
    std::cout << "recv cmd:" << _dh->cmd << std::endl;
    switch (_dh->cmd)
    {
    case LOGIN:
    {
        Login *login = (Login *)(_read_buffer);
        std::cout << "login " << login->name << " :" << login->password;
        break;
    }
    case LOGINRET:
    {
        LoginRet *loginret = (LoginRet *)(_read_buffer);
        std::cout << "loginret " << loginret->code;
        break;
    }
    case LOGOUT:
    {
        Logout *logout = (Logout *)(_read_buffer);
        std::cout << "logout " << logout->name;
        break;
    }
    case LOGOUTRET:
    {
        LogoutRet *logoutret = (LogoutRet *)(_read_buffer);
        std::cout << "logoutret " << logoutret->code;
        break;
    }
    case CHAT:
    {
        ChatMsg *chatmsg = (ChatMsg *)(_read_buffer);
        std::cout << "chatmsg " << chatmsg->name << ":" << chatmsg->msg << std::endl;
        break;
    }
    case CHATRET:
    {
        ChatMsgRet *chatmsgret = (ChatMsgRet *)(_read_buffer);
        std::cout << "chatmsgret " << chatmsgret->code;
        break;
    }
    case ERROR:
    {
        break;
    }

    default:
        std::cout << "unexpected proto type:" << _dh->cmd << std::endl;
        break;
    }
    return 0;
}

EasyEllConn::~EasyEllConn() {
    if (!isClosed())
    {
        onCloseFd();
        close();
    }
    delete (_ec);
    std::cout << "net closed" << std::endl;
}