#pragma once

#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <sys/event.h>
#include <unordered_map>

#define EXITSTR "EXIT"
#define STOPSTR "STOP"
#define BUFFSIZE 1024
#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 8888

// char brocastbuff[BUFFSIZE];