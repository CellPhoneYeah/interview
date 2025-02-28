class EventHandler;
class NetServerFactory{
    public:
    static EventHandler * createEpollEventHandler();
    static void handleAcceptEvent(int fd, EventType et);
    static void handleAcceptCallback(int fd, EventType et, EventContext &ec);
};