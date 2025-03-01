#include "EpollEventContext.h"
#include <unordered_map>
class EpollContextManager{
    public:
    static void addContext(EpollEventContext*);
    static void delContext(int fd);
    static EpollEventContext* getContext(int fd);
    private:
    static std::unordered_map<int, EpollEventContext*> contexts;
};