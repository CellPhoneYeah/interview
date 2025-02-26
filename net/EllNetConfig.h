#define INVALID_SOCK (~0);

typedef struct EllNetConfig{
public:
    const static int RING_BUFFER_SIZE = 10240;
    const static int READ_BUFFER_SIZE = 1024;
    const static int WRITE_BUFFER_SIZE = 64;
    const static int RITE_QUEUE_SIZE = 1024;
    const static int IP_ADDRESS_LEN = 40;
    const static int MAX_LISTEN_CONN = 1024;
} ENConfig;

enum EVENTLIST{
    READ_EVENT,
    WRITE_EVENT
};