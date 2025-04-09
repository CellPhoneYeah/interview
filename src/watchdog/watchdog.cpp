#include "watchdog/watchdog.h"
#include <sstream>

namespace ellnet
{
    std::unordered_map<int, std::shared_ptr<WatchDog>> WatchDog::all_dogs_;
    std::mutex WatchDog::all_dogs_mutex_;

    WatchDog::WatchDog(WatchDog *pWD)
    {
        this->connections_ = std::unordered_set<int>(pWD->connections_);
    }

    WatchDog::WatchDog(const int session_id)
    {
        this->session_id_ = session_id;
        this->connections_.clear();
    }

    WatchDog::~WatchDog()
    {
        this->connections_.clear();
    }

    std::shared_ptr<WatchDog> WatchDog::GetDog(const int session_id)
    {
        std::lock_guard<std::mutex> lock(all_dogs_mutex_);
        auto it = all_dogs_.find(session_id);
        return it != all_dogs_.end() ? it->second : nullptr;
    }

    void WatchDog::PutDog(WatchDog *pDog)
    {
        std::lock_guard<std::mutex> lock(all_dogs_mutex_);
        std::shared_ptr<WatchDog> spDog = std::make_shared<WatchDog>(pDog);
        all_dogs_.insert(std::make_pair(pDog->GetSessionId(), spDog));
    }

    void WatchDog::DelDog(const int session_id)
    {
        std::lock_guard<std::mutex> lock(all_dogs_mutex_);
        all_dogs_.erase(session_id);
    }

    void WatchDog::Clean()
    {
        all_dogs_.clear();
    }

    std::string WatchDog::ToString()
    {
        std::ostringstream oss;
        oss.clear();
        oss << "all_dogs_:(";
        for (auto d : all_dogs_)
        {
            oss << d.second << ",";
        }
        oss << ")";
        return oss.str();
    }

    int WatchDog::GetConn(const int fd)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto it = connections_.find(fd);
        return it == connections_.end() ? *it : -1;
    }

    void WatchDog::PutConn(const int fd)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.insert(fd);
    }

    void WatchDog::DelConn(const int fd)
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(fd);
    }

    std::ostream &operator<<(std::ostream &os, const WatchDog &wd)
    {
        std::ostringstream oss;
        oss << "(";
        for (auto it : wd.GetConnections())
        {
            oss << it << "|";
        }
        oss << ")";
        os << "watch dog<session_id:" << wd.GetSessionId() << "connections:" << oss.str() << ">";
        return os;
    }
}
