# before using
need g++ version >= 17
in linux
```
sudo apt install protobuf-compiler libprotobuf-dev
```
in macos
```
brew install protobuf
```
# using
- using vscode open the root folder
- run the launch task named "server Launch" or "client Launch" to test the socket code
- run the launch task named "test serialize" to test serialize code

# net work state machine
```mermaid
stateDiagram-v2
    [*] --> LISTEN_WAIT_OPEN: CMD_INIT_LISTEN
    [*] --> CONNECT_WAIT_OPEN: CMD_INIT_CONNECT
    [*] --> PIPE_LISTENING: init
    PIPE_LISTENING --> CLOSED: close
    CONNECT_WAIT_OPEN --> CONNECTED: CMD_START_CONNECT
    CONNECT_WAIT_OPEN --> CLOSED: CMD_CLOSE
    LISTEN_WAIT_OPEN --> CLOSED: CMD_CLOSE
    LISTEN_WAIT_OPEN --> LISTENING: CMD_START_LISTEN
    LISTENING --> CLOSED: CMD_CLOSE
    CONNECTED --> WRITING_BEFORE_CLOSE: CMD_CLOSE
    WRITING_BEFORE_CLOSE --> CLOSED: mq is empty
```