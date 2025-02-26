#include <iostream>
#include <string.h>
#include <fstream>
#include "proto/generated/proto/login.pb.h"


int main(){
    std::cout << "ok" << std::endl;
    login::Login login;
    login.set_name("yexiaofeng");
    login.set_password("123");
    login.set_id(44);
    char buff[1024];
    login.SerializeToArray(buff, 1024);
    std::cout << "after serialize " << buff << std::endl;
    login::Login out;
    out.ParseFromArray(buff, 1024);
    std::cout << "after parse serialize " << out.ShortDebugString() << std::endl;
    return 0;
}