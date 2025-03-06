#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include "slog.h"
#include <unordered_set>

std::unordered_set<int> fdsets;

int main(){
    int epoll_fd = epoll_create1(0);

    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int opt = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0){
        SPDLOG_INFO("sock set reuse failed {} !", strerror(errno));
        return -6;
    }
    struct sockaddr_in listen_addr;
    socklen_t len = sizeof(listen_addr);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    listen_addr.sin_port = htons(8088);
    if(bind(listenfd, (sockaddr*)&listen_addr, len) < 0){
        SPDLOG_INFO("sock bind failed:{}", strerror(errno));
        close(listenfd);
        return -2;
    }

    if(listen_addr.sin_addr.s_addr == INADDR_NONE){
        SPDLOG_INFO("invalid ip addr ");
        close(listenfd);
        return -4;
    }

    if(listen(listenfd, 128) < 0){
        SPDLOG_INFO("listen failed ", strerror(errno));
        return -5;
    } 

    struct epoll_event ls_ev;
    ls_ev.events = EPOLLIN;
    ls_ev.data.fd = listenfd;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &ls_ev) < 0){
        SPDLOG_INFO("listen failed {}", strerror(errno));
        return -2;
    }

    struct epoll_event ev[1024];
    struct epoll_event *curev = nullptr;
    SPDLOG_INFO("start listening {}", listenfd);
    while(1){
        int eventn = epoll_wait(epoll_fd, ev, 1024, 100);
        if(eventn < 0){
            SPDLOG_ERROR("wait err {}", strerror(errno));
            return -3;
        }
        if(eventn == 0){
            continue;
        }
        for(int i = 0; i < eventn; i++){
            struct epoll_event curev = ev[i];
            if(curev.data.fd == listenfd){
                while(1){
                    struct sockaddr_in addr;
                    socklen_t len = sizeof(addr);
                    int flag = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
                    int newFd = accept(listenfd, (sockaddr*)&addr, &len);
                    if(newFd > 0){
                        curev.data.fd = newFd;
                        curev.events = EPOLLIN | EPOLLET;
                        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newFd,  &curev) < 0){
                            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, newFd, &curev);
                        }
                        if(fdsets.find(newFd) != fdsets.end()){
                            SPDLOG_WARN("repeat fd!!! {}", newFd);
                        }
                        fdsets.insert(newFd);
                        SPDLOG_INFO("accept new fd {} {}", (int)newFd, fdsets.size());
                        continue;
                    }
                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                        SPDLOG_INFO("finished one accept ");
                        break;
                    }
                    SPDLOG_ERROR("accept err {}", strerror(errno));
                    return 0;
                }
            }else if(curev.events & EPOLLIN){
                char buff[1024];
                int n = recv(curev.data.fd, buff, 1024, 0);
                if(n == 0){
                    close(curev.data.fd);
                    fdsets.erase(curev.data.fd);
                    SPDLOG_INFO("close fd {} {}", (int)curev.data.fd, fdsets.size());
                }else{
                    SPDLOG_INFO("recv data {}", buff);
                }
            }else{
                SPDLOG_INFO("not handle event type {}", (int)curev.data.fd);
            }
        }
    }
}