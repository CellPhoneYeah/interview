#include "watchdog.h"
#include <sstream>

std::unordered_map<int, std::shared_ptr<watchdog>> watchdog::allDogs;
std::mutex watchdog::allDogs_mutex;

watchdog::watchdog(watchdog *pWD)
{
    this->connections = std::unordered_set<int>(pWD->connections);
}

watchdog::watchdog(int session_id)
{
    this->session_id = session_id;
    this->connections.clear();
}

watchdog::~watchdog()
{
    this->connections.clear();
}

std::shared_ptr<watchdog> watchdog::getDog(int session_id)
{
    std::lock_guard<std::mutex> lock(allDogs_mutex);
    auto it = allDogs.find(session_id);
    return it != allDogs.end() ? it->second : nullptr;
}

void watchdog::putDog(watchdog * pDog)
{
    std::lock_guard<std::mutex> lock(allDogs_mutex);
    std::shared_ptr<watchdog> spDog = std::make_shared<watchdog>(pDog);
    allDogs.insert(std::make_pair(pDog->getSessionId(), spDog));
}

void watchdog::delDog(int session_id)
{
    std::lock_guard<std::mutex> lock(allDogs_mutex);
    allDogs.erase(session_id);
}

void watchdog::clean()
{
    allDogs.clear();
}

std::string watchdog::toString()
{
    std::ostringstream oss;
    oss.clear();
    oss << "allDogs:(";
    for(auto d: allDogs){
        oss << d.second << ",";
    }
    oss << ")";
    return oss.str();
}

int watchdog::getConn(int fd)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    auto it = connections.find(fd);
    return it == connections.end() ? *it : -1;
}

void watchdog::putConn(int fd)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    connections.insert(fd);
}

void watchdog::delConn(int fd)
{
    std::lock_guard<std::mutex> lock(connections_mutex);
    connections.erase(fd);
}

std::ostream& operator<<(std::ostream& os, const watchdog &wd)
{
    std::ostringstream oss;
    oss << "(";
    for(auto it : wd.getConnections()){
        oss << it << "|";
    }
    oss << ")";
    os << "watch dog<session_id:"<< wd.getSessionId() << "connections:"<< oss.str() << ">";
    return os;
}