#ifndef WATCH_DOG_H
#define WATCH_DOG_H
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include "slog.h"
#include <string>
namespace ellnet
{
    class WatchDog
    {
    private:
        static std::mutex all_dogs_mutex_;
        static std::unordered_map<int, std::shared_ptr<WatchDog>> all_dogs_;

        std::mutex connections_mutex_;
        int session_id_;
        std::unordered_set<int> connections_;

    public:
        WatchDog(WatchDog *);
        WatchDog(const int session_id);
        ~WatchDog();

        static std::shared_ptr<WatchDog> GetDog(const int session_id);
        static void PutDog(WatchDog *);
        static void DelDog(const int session_id);
        static void Clean();
        static std::string ToString();

        int GetConn(const int fd);
        void PutConn(const int fd);
        void DelConn(const int fd);
        int GetSessionId() const { return this->session_id_; }
        std::unordered_set<int> GetConnections() const { return this->connections_; }
        friend std::ostream &operator<<(std::ostream &, const WatchDog &wd);
    };
}

#endif