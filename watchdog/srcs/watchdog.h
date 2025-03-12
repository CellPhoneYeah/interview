#ifndef WATCH_DOG_H
#define WATCH_DOG_H
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include "slog.h"
#include <string>
class watchdog{
private:
    static std::mutex allDogs_mutex;
    static std::unordered_map<int, std::shared_ptr<watchdog>> allDogs;

    std::mutex connections_mutex;
    int session_id;
    std::unordered_set<int> connections;
public:
    watchdog(watchdog*);
    watchdog(int session_id);
    ~watchdog();
    static std::shared_ptr<watchdog> getDog(int session_id);
    static void putDog(watchdog*);
    static void delDog(int session_id);
    static void clean();
    static std::string toString();

    int getConn(int fd);
    void putConn(int fd);
    void delConn(int fd);
    int getSessionId()const {return this->session_id;}
    std::unordered_set<int> getConnections() const{return this->connections;}
    friend std::ostream& operator<<(std::ostream&, const watchdog& wd);
};
#endif