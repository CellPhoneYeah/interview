#include "utility.h"
#include "user.h"
#include "proto.h"

int main(){
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){
        cout << " create fd fail" << endl;
        exit(-1);
    }
    struct sockaddr_in server_addr; 
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr(SERVER_HOST);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if( bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind err");
        exit(-1);
    }
    
    int listenret = listen(listenfd, MAX_CANON);
    if(listenret < 0){
        perror("listen error");
        close(listenfd);
        exit(-1);
    }

    struct sockaddr_in client_addr;
    socklen_t clientaddr_len = sizeof(struct sockaddr_in);
    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;

    int kq = kqueue();
    if(kq < 0){
        perror("kq create err");
        close(listenfd);
        exit(-1);
    }
    struct kevent change_event, event_list[BUFFSIZE];
    // 事件参数，监听端口，关注的事件类型，处理方式（添加并启用）,过滤器标记,过滤器特定参数,用户数据
    EV_SET(&change_event, listenfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);

    if(kevent(kq, &change_event, 1, nullptr, 0, nullptr) < 0){
        perror("kevent error");
        close(listenfd);
        close(kq);
        exit(-1);
    }
    cout << "start kqueue" << endl;
    while (true)
    {
        int changen = kevent(kq, nullptr, 0, event_list, BUFFSIZE, nullptr);
        if(changen < 0){
            perror("accept error");
            close(listenfd);
            close(kq);
            exit(-1);
        }
        for (size_t i = 0; i < changen; i++)
        {
            if(event_list[i].ident == listenfd){
                int clientfd = accept(listenfd, (sockaddr*)&client_addr, &clientaddr_len);
                if(clientfd < 0){
                    perror("accept err");
                }else{
                    cout << inet_ntoa(client_addr.sin_addr) << " connect in with port " << ntohs(client_addr.sin_port) << endl;
                    EV_SET(&change_event, clientfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
                    if(kevent(kq, &change_event, 1, nullptr, 0, nullptr) < 0){
                        perror("register client error");
                        close(clientfd);
                        delFromClientFds(clientfd);
                    }
                    addToClientFds(clientfd);
                }
            }else{
                // client data
                //sendBrocastMessage(event_list[i].ident);
                if(handleProto(event_list[i].ident) < 0){
                   close(event_list[i].ident);
                   delFromClientFds(event_list[i].ident);
                }
            }
        }
    }
    close(listenfd);
    close(kq);
    return 0;
}