#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



void *control(void *sck) {
    int readSock=(int)sck;
    char* arr[9];
    char buff[1024];
    while(1) {
        int size=read(readSock, buff, 1024);
        if(size==-1) {
            perror("Error reading");
        }
        else if(size==0) {
            continue;
        }
        buff[size]='\0';
        if(buff[size-1]=='\n') {
            buff[size-1]='\0';
        }
        int i=0;
        arr[0]=strtok(buff, " ");
        while(arr[i]!=NULL&&i<9) {
            i++;
            arr[i]=strtok(NULL, " ");
        }
        if(i==0) {
            continue;
        }
        if(strcmp(arr[0], "echo")==0) {
            int j=1;
            while(arr[j]!=NULL) {
                write(STDOUT_FILENO, " ", 1);
                write(STDOUT_FILENO, arr[j], strlen(arr[j]));
                j++;
            }
            write(STDOUT_FILENO, "\n", 1);
        }
        else if(strcmp(arr[0], "disconnect")==0) {
            write(STDOUT_FILENO, "Exiting..\n", 10);
            exit(0);
        }
    }
}

void *socket2stdout(void *ar) {
    char buff[1024];
    int readSock=(int)ar;
    while(1) {
        int fg=read(readSock, (char*)buff, 1024);
        if(fg==-1) {
            perror("Error reading");
        }
        write(STDOUT_FILENO, buff, fg);
    }
}



void printHelp() {
    write(STDOUT_FILENO, "connect [IP] [PORT]\n", 47-27);
    write(STDOUT_FILENO, "run [EXECUTABLE_FILE]\n", 49-27);
    write(STDOUT_FILENO, "add [N1] [N2]\n", 41-27);
    write(STDOUT_FILENO, "sub [N1] [N2]\n", 41-27);
    write(STDOUT_FILENO, "mult [N1] [N2]\n", 42-27);
    write(STDOUT_FILENO, "div [N1] [N2]\n", 41-27);
    write(STDOUT_FILENO, "kill -n [PROCESS_NAME]\n", 50-27);
    write(STDOUT_FILENO, "kill -P [PROCESS_PID]\n", 49-27);
    write(STDOUT_FILENO, "kill -a\n", 35-27);
}

int main(int argc, char *argv[]) {
    /*if(argc<3) {
        write(STDOUT_FILENO, "Too few arguments\n", 18);
        return -1;
    }*/
    printHelp();
    int sock;
	while(1) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("opening stream socket");
            exit(-1);
        }
        char bkp[23];
        struct sockaddr_in server;
        while(1) {
            char fbuff[64];
            int rsiz=read(STDIN_FILENO, fbuff, 64);
            fbuff[rsiz]='\0';
            if(fbuff[rsiz-1]=='\n') {
                fbuff[rsiz-1]='\0';
            }
            char* arr3[9];
            int l=0;
            arr3[0]=strtok(fbuff, " ");
            while(arr3[l]!=NULL&&l<9) {
                l++;
                arr3[l]=strtok(NULL, " ");
            }
            if(l==0) {
                continue;
            }
            if(strcmp(arr3[0], "help")==0) {
                printHelp();
                continue;
            }
            if(strcmp(arr3[0], "help")==0) {
                close(sock);
                exit(0);
            }
            if(strcmp(arr3[0], "connect")==0) {
                if(l<3) {
                    continue;
                }
                server.sin_family = AF_INET;
                struct hostent *hp = gethostbyname(arr3[1]);
                strcpy(bkp, arr3[1]);
                if (hp == 0) {
                    write(STDOUT_FILENO, "Could not connect\n", 61-43);
                    continue;
                }
                bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
                server.sin_port = htons(atoi(arr3[2]));
                if(connect(sock,(struct sockaddr *) &server,sizeof(server)) < 0) {
                    perror("connecting stream socket");
                    continue;
                }
                else {
                    break;
                }
                write(STDOUT_FILENO, "Unrecognized command\n", 60-39);
            }
            
        }
        struct sockaddr_in controlServer;
        controlServer.sin_family = AF_INET;
        /*//controlServer.sin_port = htons(6667);
        //controlServer.sin_addr.s_addr = inet_addr("192.168.43.203");
        struct hostent *hp2 = gethostbyname(bkp);
        if (hp2 == 0) {
            write(STDOUT_FILENO, "Bad host2\n", 10);
            exit(-1);
        }
        bcopy(hp2->h_addr, &controlServer.sin_addr, hp2->h_length);*/
        controlServer.sin_addr.s_addr=INADDR_ANY;
        controlServer.sin_port = 0;
        int readSock = socket(AF_INET, SOCK_STREAM, 0);
        
        if (bind(readSock, (struct sockaddr *) &controlServer, sizeof(controlServer))==-1) {
            perror("Error binding socket2");
            exit(-1);
        }
        int length = sizeof(controlServer);
        if (getsockname(readSock, (struct sockaddr *) &controlServer, (socklen_t*) &length)) {
            perror("Error getsockname");
            exit(-1);
        }
        listen(readSock, 2);
        
        char strport[14];
        int n=sprintf(strport, "client %d", ntohs(controlServer.sin_port));
        if(write(sock, strport, n) == -1) {
            perror("Error writing on socket");
            exit(-1);
        }
        struct sockaddr_in client;
        int cSize=sizeof(client);
        int serverSock = accept(readSock, (struct sockaddr*)&client, &cSize);
        if(serverSock==-1) {
            perror("Error accepting connection");
            exit(-1);
        }
        pthread_t print_thread;
        if(pthread_create(&print_thread, NULL, socket2stdout, (void*)sock)) {
            write(STDOUT_FILENO, "print_thread error\n", 19);
            exit(-1);
        }
        
        pthread_t control_thread;
        if(pthread_create(&control_thread, NULL, control, (void*)serverSock)) {
            write(STDOUT_FILENO, "control_thread error\n", 21);
            exit(-1);
        }
        char input[1024];
        while(1) {
            int size=read(STDIN_FILENO, input, 1024);
            if(size==-1) {
                perror("Error writing on socket");
            }
            if(size==0) {
                continue;
            }
            if(input[0]=='\n') {
                continue;
            }
            input[size-1]='\0';
            if(strcmp(input, "help")==0) {
                printHelp();
                continue;
            }
            else if(strcmp(input, "exit")==0) {
                close(sock);
                exit(0);
            }
            else if(strcmp(input, "disconnect")==0) {
                break;
            }
            if(write(sock, input, size) == -1) {
                perror("Error writing on socket");
            }
        }
        write(STDOUT_FILENO, "Disconnected\n", 44-31);
        close(sock);
    }
}
