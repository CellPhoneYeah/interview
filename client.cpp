#include "utility.h"
#include <thread>
#include "EllBaseServer.h"

void sendCmd(int pipe_fd, bool isworking, int&islogin, int& islogout)
{
    std::cout << " pipe fd:" << pipe_fd << std::endl;
    struct Login login;
    std::cout << "input name:";
    char name[32];
    fgets(name, 32, stdin);
    for (int i = 0; i < 32; i++)
    {
        if (name[i] == '\n')
        {
            name[i] = '\0';
        }
    }
    std::cout << "input password:";
    char password[32];
    fgets(password, 32, stdin);
    
    std::strcpy(login.name, name);
    std::strcpy(login.password, password);
    std::cout << "name:" << login.name << " password:" << login.password << std::endl;

    while (isworking)
    {
        if (islogin == 0)
        {
            write(pipe_fd, (const char *)&login, sizeof(login));
            std::cout << "send login to pipe" << std::endl;
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
        else if(islogout == 2){
            break;
        }
        else
        {
            char buffer[BUFFSIZE];
            bzero(&buffer, BUFFSIZE);
            std::cout << "write some message:";
            fgets(buffer, BUFFSIZE, stdin);
            std::cout << "gets :" << std::string(buffer) << std::endl;
            if (strncasecmp(buffer, EXITSTR, strlen(EXITSTR)) == 0)
            {
                std::cout << " closed" << std::endl;
                struct Logout logout;
                strcpy(login.name, logout.name);
                write(pipe_fd, (const char *)&logout, logout.dataLen);
                islogout = 1;
            }
            else
            {
                // cout << "before write :" << string(buffer) << endl;
                struct ChatMsg chatmsg;
                strcpy(chatmsg.name, login.name);
                strcpy(chatmsg.msg, buffer);
                write(pipe_fd, (const char *)&chatmsg, chatmsg.dataLen);
                std::cout << "write in pipe: " << std::string(chatmsg.msg) << std::endl;
            }
        }
    }
    close(pipe_fd);
    std::cout << "client sender close" << std::endl;
}

void checkKqueue(int pipe_fd, EasyEllConn *eec, int kq, int& isworking, int&islogin, int& islogout){
    std::cout << "reading isworking" << pipe_fd << " " << kq << " " << eec->getSock() << std::endl;
    struct kevent event[1024];
    while (isworking)
    {
        if(eec->loopListenSock(event, 1024) < 0){
            break;
        }
    }
    std::cout << "client reader close" << std::endl;
}

void loopConn(bool &isRunning){
    EllBaseServer *ebs = new EllBaseServer();
    if(ebs->connectTo(SERVER_HOST, SERVER_PORT) < 0){
        std::cout << "connect failed" << errno << std::endl;
        return;
    }

    while(isRunning && ebs->loopKQ() > 0){
        sleep(1);
        std::cout << "client looping" << std::endl;
    }
    delete(ebs);
}

int main()
{
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_addr.s_addr = inet_addr(SERVER_HOST);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVER_PORT);

    int kq = kqueue();
    int pipe_fd[2];
    pipe(pipe_fd);
    EasyEllConn *eec = new EasyEllConn(kq);
    if (eec->connect(SERVER_HOST, SERVER_PORT) < 0)
    {
        perror("connect err");
        exit(-1);
    }
    eec->registerReadEv();
    EasyEllConn::addClient(eec->getSock(), eec);
    EventContext *ec = new EventContext();
    ec->socket_type = SOCKET_TYPE_PIPE;
    ec->targetfd = eec->getSock();
    struct kevent event_change;
    EV_SET(&event_change, pipe_fd[0], EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, ec);
    kevent(kq, &event_change, 1, nullptr, 0, nullptr);

    int isworking = 1;
    int islogout = 0;
    int islogin = 0;
    // std::thread th(sendCmd, pipe_fd[1], std::ref(isworking), std::ref(islogin), std::ref(islogout));
    // th.detach();
    // std::thread th_read(checkKqueue, pipe_fd[0], eec, kq, std::ref(isworking), std::ref(islogin), std::ref(islogout));
    // th_read.join();
    bool isRunning;
    std::thread th_loop(loopConn, std::ref(isRunning));
    th_loop.detach();
    sendCmd(pipe_fd[1], isRunning, islogin, islogout);

    return 0;
}