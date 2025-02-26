#include <unordered_map>
class EllConn;

class ConnManager{
private:
    static std::unordered_map<int, EllConn*> clientMap;
public:
    static EllConn* getClient(int sockFd);
    static void addClient(int sockFd, EllConn* ec);
    static void delClient(int sockFd);
}