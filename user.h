#include <string>
#include <iostream>
#include <map>
#include <set>

using namespace std;

map<string, string> userpwmap;
set<string> loginList;

class user{
private:
    string name;
    int age;
    string password;
    int sockfd;
public:
    bool hasLogin(string name){
        map<string, string>::iterator it = userpwmap.find(name);
        return it != userpwmap.end();
    }
    user(string name, int age, string pw){
        this->name = name;
        this->age = age;
        this->password = pw;
    }
    int login(string name, string pw){
        if (loginList.contains(name))
        {
            return 1;
        }
        
        map<string, string>::iterator it = userpwmap.find(name);
        if(it == userpwmap.end()){
            cout << "not found user" << endl;
            return -1;
        }
        if(name.compare((*it).first) != 0){
            return -2;
            
        }
        if(pw.compare((*it).second) != 0){
            return -3;
        }
        loginList.insert(name);
        return 1;
    }

    void logout(string name){
        userpwmap.erase(name);
    }
    void setSockFd(int sockfd){
        this->sockfd = sockfd;
    }
};