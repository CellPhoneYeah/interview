#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include "slog.h"
#include <unordered_set>
#include <thread>

std::unordered_set<int> fdsets;
int epoll_fd = epoll_create1(0);

void create_conn(){
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    int errcode;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8088);
    socklen_t sz = sizeof(addr);

    struct epoll_event ls_ev;
    ls_ev.events = EPOLLOUT;
    ls_ev.data.fd = clientfd;

    if(connect(clientfd, (sockaddr *)&addr, sz) < 0){
        if(errno != EINPROGRESS){
            SPDLOG_INFO("conn failed ", strerror(errno));
            return;
        }
    }else{
        ls_ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientfd, &ls_ev);
        fdsets.insert(clientfd);
        SPDLOG_INFO("conn success {} {}", clientfd, fdsets.size());
        return;
    }

    SPDLOG_INFO("start clientfd {}", clientfd);
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientfd, &ls_ev) < 0){
        SPDLOG_INFO("connect failed {}", strerror(errno));
        return;
    }
}

void loop(){
    struct epoll_event ev_list[1024];
    while(1){
        int eventn = epoll_wait(epoll_fd, ev_list, 1024, 100);
        if(eventn < 0){
            SPDLOG_ERROR("wait err {}", strerror(errno));
            return;
        }
        if(eventn == 0){
            continue;
        }
        for(int i = 0; i < eventn; i++){
            struct epoll_event curev = ev_list[i];
            if(curev.events & EPOLLIN){
                char buff[1024];
                while(1){
                    int n = recv(curev.data.fd, buff, 1024, 0);
                    if(n == 0){
                        close(curev.data.fd);
                        SPDLOG_INFO("close fd {}", (int)curev.data.fd);
                        fdsets.erase(curev.data.fd);
                        break;
                    }else if (n < 0){
                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                            break;
                        }else{
                            SPDLOG_ERROR("recv err {}", strerror(errno));
                            close(curev.data.fd);
                            break;
                        }
                    }
                    SPDLOG_INFO("recv data {}", buff);
                }
            }
            if(curev.events & EPOLLOUT){
                int errcode;
                socklen_t len = sizeof(errcode);
                int cur_fd = curev.data.fd;
                if(getsockopt(curev.data.fd, SOL_SOCKET, SO_ERROR, &errcode, &len) == -1){
                    SPDLOG_INFO("connect failed {}", strerror(errno));
                    continue;
                }
                if(errcode != 0){
                    SPDLOG_INFO("connected failed ", errcode);
                    continue;
                }
                curev.events = EPOLLIN | EPOLLET;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, curev.data.fd, &curev) < 0){
                    SPDLOG_WARN(" add to epoll failed {}", (int)curev.data.fd);
                    continue;
                }
                fdsets.insert(curev.data.fd);
                SPDLOG_INFO("handle connect {} {}", cur_fd, fdsets.size());
            }
        }
    }
}

int main(){
    for(int i = 0; i < 100; i++){
        create_conn(); // 必须保证使用 epoll_ctl 时没有竞争状态，否则会出现数据异常
    }
    std::thread th_loop(loop);
    th_loop.join();
    return 0;
}