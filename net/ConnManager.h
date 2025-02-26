#include <unordered_map>
class EllConnBase;
class ConnManager{
private:
    static std::unordered_map<int, EllConnBase*> clientMap;
public:
    static EllConnBase* getClient(int sockFd);
    static void addClient(int sockFd, EllConnBase* ecb);
    static void delClient(int sockFd);
};