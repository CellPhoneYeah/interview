#include "utility.h"
#include <thread>
#include "EllBaseServer.h"

void sendCmd(int pipe_fd, bool &isworking, int&islogin, int& islogout, EllBaseServer* ebs, int pipe_fd_out)
{
    std::cout << " pipe fd:" << pipe_fd << std::endl;
    struct Login login;
    std::cout << "input name:";
    char name[32];
    char* ret = fgets(name, 32, stdin);
    if(ret == nullptr){
        std::cout << " insert name failed\n";
        return;
    }
    for (int i = 0; i < 32; i++)
    {
        if (name[i] == '\n')
        {
            name[i] = '\0';
        }
    }
    std::cout << "input password:";
    char password[32];
    ret = fgets(password, 32, stdin);
    if(ret == nullptr){
        std::cout << " insert password failed\n";
        return;
    }
    
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
            isworking = false;
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
                ebs->getConn(pipe_fd_out)->registerReadEv();
                islogout = 1;
            }
            else
            {
                // cout << "before write :" << string(buffer) << endl;
                struct ChatMsg chatmsg;
                strcpy(chatmsg.name, login.name);
                strcpy(chatmsg.msg, buffer);
                write(pipe_fd, (const char *)&chatmsg, chatmsg.dataLen);
                ebs->getConn(pipe_fd_out)->registerReadEv();
                std::cout << "write in pipe: " << std::string(chatmsg.msg) << std::endl;
            }
        }
    }
    close(pipe_fd);
    std::cout << "client sender close" << std::endl;
}

void loopConn(bool &isRunning, int pipe_fd_in, EllBaseServer* ebs){
    if(ebs->connectTo(SERVER_HOST, SERVER_PORT) < 0){
        std::cout << "connect failed" << errno << std::endl;
        return;
    }
    ebs->newPipe(pipe_fd_in);

    int loopCount = 0;
    while(isRunning && ebs->loopKQ() >= 0){
        sleep(1);
        loopCount ++;
        if(loopCount % 10 == 0){
            std::cout << "client looping" << *ebs << std::endl;
            loopCount = 0;
        }
    }
    std::cout << "stop loop" << std::endl;
    delete(ebs);
}

int main()
{
    int pipe_fd[2];
    pipe(pipe_fd);

    int islogout = 0;
    int islogin = 0;
    bool isRunning = true;
    EllBaseServer *ebs = new EllBaseServer();
    std::thread th_loop(loopConn, std::ref(isRunning), pipe_fd[0], ebs);
    th_loop.detach();
    sendCmd(pipe_fd[1], std::ref(isRunning), std::ref(islogin), std::ref(islogout), ebs, pipe_fd[0]);

    return 0;
}