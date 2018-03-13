# Remote Terminal
## Class project for System Programming
### Run any command on the server remotely using sockets.
The server program waits for client(on a network) or local keyboard input.
#### Supported commands for client are:
- run [EXECUTABLE_FILE]
- add [n1...n2]
- sub [n1...n2]
- mult [n1...n2]
- div [N1] [N2]
- kill [-a -n NAME -p PID]
- list
- list [-a]
- exit

#### Supported local commands for client are:
- help
- connect [IP PORT]
- disconnect
- exit

#### Supported local commands for server are:
- list [-c(clients) -p(processes)]
- kill [-a(all) -n NAME -p PID]
- kick [IP:PORT]
- msg [IP:PORT] [MESSAGE]
- broadcast [MESSAGE]
- exit
