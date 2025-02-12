#include "utility.h"
#include "proto.h"

int main()
{
    std::cout << "input name:";
    char name[32];
    fgets(name, 32, stdin);
    for(int i = 0; i < 32; i++){
        if(name[i] == '\n'){
            name[i] = '\0';
        }
    }
    std::cout << "input password:";
    char password[32];
    fgets(password, 32, stdin);
    struct Login login;
    std::strcpy(login.name, name);
    std::strcpy(login.password, password);
    std::cout << "name:" << login.name << " password:" << login.password << std::endl;
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_addr.s_addr = inet_addr(SERVER_HOST);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVER_PORT);

    int kq = kqueue();
    int pipe_fd[2];
    pipe(pipe_fd);
    struct kevent event_change;
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(clientfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("connect err");
        exit(-1);
    }
    EV_SET(&event_change, clientfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    kevent(kq, &event_change, 1, nullptr, 0, nullptr);
    EV_SET(&event_change, pipe_fd[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    kevent(kq, &event_change, 1, nullptr, 0, nullptr);

    int isworking = 1;
    int islogout = 0;
    int islogin = 0;
    int pid = fork();
    if (pid == 0)
    {
        // 子进程
        close(pipe_fd[0]);
        while (isworking)
        {
            if (islogin == 0)
            {
                write(pipe_fd[1], (const char *)&login, sizeof(login));
                islogin = 1;
            }
            else if (islogin == 1)
            {
                std::cout << "waiting login..." << std::endl;
                sleep(1);
                islogin = 2;
            }
            else if (islogout == 1)
            {
                std::cout << "waiting logout..." << std::endl;
                sleep(1);
                islogout = 2;
            }
            else
            {
                char buffer[BUFFSIZE];
                bzero(&buffer, BUFFSIZE);
                cout << "write some message:";
                fgets(buffer, BUFFSIZE, stdin);
                // cout << "gets :" << string(buffer) << endl;
                if (strncasecmp(buffer, EXITSTR, strlen(EXITSTR)) == 0)
                {
                    cout << " closed" << endl;
                    struct Logout logout;
                    strcpy(login.name, logout.name);
                    write(pipe_fd[1], (const char *)&logout, logout.dataLen);
                    islogout = 1;
                }
                else
                {
                    // cout << "before write :" << string(buffer) << endl;
                    struct ChatMsg chatmsg;
                    strcpy(chatmsg.name, login.name);
                    strcpy(chatmsg.msg, buffer);
                    write(pipe_fd[1], (const char *)&chatmsg, chatmsg.dataLen);
                    // cout << "write in pipe: " << string(buffer) << endl;
                }
            }
        }
    }
    else
    {
        // 父进程
        close(pipe_fd[1]);
        struct kevent event;
        char message[BUFFSIZE];
        while (isworking)
        {
            int len = kevent(kq, nullptr, 0, &event, 1, nullptr);
            if (event.ident == clientfd)
            {
                char byterecv[1024];
                bzero(byterecv, 1024);
                ssize_t len = recvHeader(clientfd, byterecv);
                if (len <= 0)
                {
                    std::cout << "server closed connection, so close client" << std::endl;
                    exit(-1);
                }
                DataHeader *dh = (DataHeader *)byterecv;
                // std::cout << "client recving" << dh->cmd << std::endl;
                switch (dh->cmd)
                {
                case LOGINRET:
                {
                    recvWithoutHeader(clientfd, byterecv, dh->dataLen);
                    LoginRet *loginret = (LoginRet *)byterecv;
                    std::cout << "loginret:" << loginret->code << std::endl;
                    if (loginret->code == 0)
                    {
                        islogin = 2;
                    }
                    break;
                }
                case LOGOUTRET:
                {
                    recvWithoutHeader(clientfd, byterecv, dh->dataLen);
                    LogoutRet *logoutret = (LogoutRet *)byterecv;
                    std::cout << "loginret:" << logoutret->code << std::endl;
                    if (logoutret->code == 0)
                    {
                        islogout = 2;
                        isworking = 0;
                    }
                    break;
                }
                case CHAT:
                {
                    recvWithoutHeader(clientfd, byterecv, dh->dataLen);
                    ChatMsg *chatmsg = (ChatMsg *)byterecv;
                    std::cout << chatmsg->name << ":" << chatmsg->msg << std::endl;
                    break;
                }
                case CHATRET:
                {
                    recvWithoutHeader(clientfd, byterecv, dh->dataLen);
                    ChatMsgRet *chatmsgret = (ChatMsgRet *)byterecv;
                    // std::cout << "message send done" << std::endl;
                    break;
                }
                default:
                    std::cout << "client unexpected msg:" << dh->cmd << " len:" << dh->dataLen << std::endl;
                    break;
                }
            }
            else
            {
                bzero(&message, BUFFSIZE);
                ssize_t len = read(pipe_fd[0], message, BUFFSIZE);
                if (len == 0)
                {
                    isworking = 0;
                }
                else
                {
                    send(clientfd, message, len, 0);
                    // cout << "send finish: " << string(message) << endl;
                }
            }
        }
    }
    if (pid == 0)
    {
        close(pipe_fd[1]);
    }
    else
    {
        close(pipe_fd[0]);
        close(clientfd);
    }

    return 0;
}