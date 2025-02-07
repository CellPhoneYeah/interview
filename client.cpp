#include "utility.h"

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
    int pid = fork();
    if(pid == 0){
        // 子进程
        close(pipe_fd[0]);
        while (isworking)
        {
            char buffer[BUFFSIZE];
            bzero(&buffer, BUFFSIZE);
            cout << "write some message:";
            fgets(buffer, BUFFSIZE, stdin);
            // cout << "gets :" << string(buffer) << endl;
            if(strncasecmp(buffer, EXITSTR, strlen(EXITSTR)) == 0){
                cout << " closed" << endl;
                isworking = 0;
            }else{
                // cout << "before write :" << string(buffer) << endl;
                if(write(pipe_fd[1], buffer, strlen(buffer) - 1) < 0){
                    perror("write err");
                    exit(-1);
                }
                // cout << "write in pipe: " << string(buffer) << endl;
            }
        }
    }else{
        // 父进程
        close(pipe_fd[1]);
        struct kevent event;
        char message[BUFFSIZE];
        while (isworking)
        {
            int len = kevent(kq, nullptr, 0, &event, 1, nullptr);
            if(event.ident == clientfd){
                cout << "recving" << endl;
                ssize_t len = recv(clientfd, message, BUFFSIZE, 0);
                if(len == 0){
                    cout << "server closed connection, so close client" << endl;
                    exit(-1);
                }
                cout << "recv:" << string(message, len) << endl;
            }else{
                bzero(&message, BUFFSIZE);
                ssize_t len = read(pipe_fd[0], message, BUFFSIZE);
                if(len == 0){
                    isworking = 0;
                }else{
                    send(clientfd, message, len, 0);
                    // cout << "send finish: " << string(message) << endl;
                }
            }
        }
    }
    if(pid == 0){
        close(pipe_fd[1]);
    }else{
        close(pipe_fd[0]);
        close(clientfd);
    }

    return 0;
}