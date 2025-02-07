#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <sys/event.h>
#include <set>

#define EXITSTR "EXIT"
#define BUFFSIZE 1024
#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 8888

using namespace std;

set<int> clientfds;

char brocastbuff[BUFFSIZE];


void delFromClientFds(int clientfd){
    if(clientfds.find(clientfd) != clientfds.end()){
        clientfds.erase(clientfd);
    }
}

void sendBrocastMessage(int sockfd){
    bzero(&brocastbuff, BUFFSIZE);
    ssize_t msglen = recv(sockfd, &brocastbuff, BUFFSIZE, 0);
    if(msglen == 0){
        cout << "connection closed" << sockfd << endl;
        close(sockfd);
        delFromClientFds(sockfd);
        return;
    }
    if(msglen < 0){
        perror("recv err");
        close(sockfd);
        delFromClientFds(sockfd);
        return;
    }
    if(clientfds.size() == 1){
        send(sockfd, &brocastbuff, msglen, 0);
    }else{
        for (set<int>::iterator i = clientfds.begin(); i != clientfds.end(); i++)
        {
            if(send(*i, &brocastbuff, msglen, 0) < 0){
                cout << "send to " << *i << " failed" << endl;
            }
        }
    }
}

void addToClientFds(int clientfd){
    if(clientfds.find(clientfd) == clientfds.end()){
        clientfds.insert(clientfd);
    }
}
