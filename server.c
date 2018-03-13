#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<signal.h>
#include<arpa/inet.h>
#include<time.h>
#define READ_SIZE 1024
#define ARG_SIZE 16

// Optimize all sizeof()'s

struct Process {
    char addr[22];
    char name[32];
    char start_time[32];
    char end_time[32];
    int pId;
    int ppId;
    int state;
    struct Process *next;
};

struct Process *head=NULL;
struct Process *last=NULL;


struct Client {
    char addr[22];
    int port;
    int pId;
    int socket;
    struct Client *next;
};

struct Client *cHead=NULL;
struct Client *cLast=NULL;

time_t rawtime;
struct tm * timeinfo;

int fd[2];
static void handler(int sig) {
    int status;
    int d=waitpid(0, &status, WNOHANG);
    struct Process *p=head;
    while(p!=NULL) {             // Consider hashing instead
        if(p->pId == d) {
            if(WIFEXITED(status)) {
                int ret=WEXITSTATUS(status);
                if(ret==-1) {
                    p->state=-1;
                }
                else {
                    p->state=0;
                }
            }
            else {
                p->state=-1;
            }
            time ( &rawtime );
            timeinfo = localtime (&rawtime);
            strcpy(p->end_time, asctime(timeinfo));
            char buff[80];
            int n=sprintf(buff, "term %s %s %d %d %s", p->name, p->addr, p->state, p->pId, p->end_time);
            write(fd[1], buff, n);
            return;
        }
        p=p->next;
    }
}

void listClients() {
    write(STDOUT_FILENO, "------------------------------------\n", 64-27);
    struct Client *p=cHead;
    while(p!=NULL) {
        char buff[128];
        int n=sprintf(buff, "%s:%d %d\n", p->addr, p->port, p->socket);
        write(STDOUT_FILENO, buff, n);
        p=p->next;
    }
    write(STDOUT_FILENO, "------------------------------------\n", 64-27);
}
void addClient(char *ip, int prt, int id, int sock) {
    if(cHead==NULL) {
        cHead=malloc(sizeof(struct Client));
        strcpy(cHead->addr, ip);
        cHead->pId=id;
        cHead->socket=sock;
        cHead->port=prt;
        cHead->next=NULL;
        cLast=cHead;
        return;
    }
    cLast->next=malloc(sizeof(struct Client));
    strcpy(cLast->next->addr, ip);
    cLast->next->pId=id;
    cLast->next->socket=sock;
    cLast->next->port=prt;
    cLast->next->next=NULL;
    cLast=cLast->next;
}
int findSockByAddr(char name[]) {
    struct Client *p=cHead;
    char buff[32];
    while(p!=NULL) {
        sprintf(buff, "%s:%d", p->addr, p->port);
        if(strcmp(buff, name)==0) {
            return p->socket;
        }
        p=p->next;
    }
    return -1;
}
int findClientPidByAddr(char name[]) {
    struct Client *p=cHead;
    char buff[32];
    while(p!=NULL) {
        sprintf(buff, "%s:%d", p->addr, p->port);
        if(strcmp(buff, name)==0) {
            return p->pId;
        }
        p=p->next;
    }
    return -1;
}
void broadcast(char msg[], int size) {
    struct Client *p=cHead;
    while(p!=NULL) {
        if(write(p->socket, msg, size)==-1) {
            perror("write");
        }
        p=p->next;
    }
}



void addProcess(char *ip, char *n, int ppid, int id, int s) {
    time ( &rawtime );
    timeinfo = localtime (&rawtime);
    if(head==NULL) {
        head=malloc(sizeof(struct Process));
        strcpy(head->addr, ip);
        strcpy(head->name, n);
        strcpy(head->start_time, asctime(timeinfo));
        head->start_time[strlen(head->start_time)-1]=' ';
        strcpy(head->end_time, " ");
        head->pId=id;
        head->ppId=ppid;
        head->state=s;
        head->next=NULL;
        last=head;
        return;
    }
    last->next=malloc(sizeof(struct Process));
    strcpy(last->next->addr, ip);
    strcpy(last->next->name, n);
    strcpy(last->next->start_time, asctime(timeinfo));
    last->next->start_time[strlen(last->next->start_time)-1]=' ';
    strcpy(last->next->end_time, " ");
    last->next->pId=id;
    last->next->ppId=ppid;
    last->next->state=s;
    last->next->next=NULL;
    last=last->next;
}
int removeProcess(int pid) {
    struct Process *p=head;
    struct Process *tmp=head;
    while(p!=NULL) {
        if(p->pId==pid) {
            tmp->next=p->next;
            p=NULL;
            return 0;
        }
        tmp=p;
        p=p->next;
    }
    return -1;
}
void listProcesses() {
    write(STDOUT_FILENO, "------------------------------------\n", 64-27);
    struct Process *p=head;
    while(p!=NULL) {
        /*char tmp[13];
        if() {
            //
        }*/
        char buff[128];
        int n=sprintf(buff, "%s   %d    %s    %d    %s    %s\n", p->addr, p->pId, p->name, p->state, p->start_time, p->end_time);
        write(STDOUT_FILENO, buff, strlen(buff));
        p=p->next;
    }
    write(STDOUT_FILENO, "------------------------------------\n", 64-27);
}
int findProcessPidByName(char *pName) {
    struct Process *p=head;
    while(p!=NULL) {
        if(strcmp(p->name, pName)==0) {
            return p->pId;
        }
        p=p->next;
    }
    return -1;
}
int setProcessState(int id, int st) {
    struct Process *p=head;
    while(p!=NULL) {
        if(p->pId==id) {
            p->state=st;
            return 0;
        }
        p=p->next;
    }
    return -1;
}
void stampEndTime(int id, char ti[]) {
    struct Process *p=head;
    while(p!=NULL) {
        if(p->pId==id) {
            strcpy(p->end_time, ti);
            return;
        }
        p=p->next;
    }
}
int genocide() {
    int count=0;
    struct Process *p=head;
    while(p!=NULL) {
        if(p->state) {
            if(kill(p->pId, SIGTERM)==0) {
                count++;
            }
        }
        p=p->next;
    }
    return count;
}


void printErrorAndExit(char msg[], int eStatus) {
    perror(msg);
    exit(eStatus);
}



                                                                                /*                      MAIN                      */

void main(int argc, char *argv[]) {
    if(pipe(fd)==-1) {
        printErrorAndExit("Error opening pipe", -1);
    }
                                                                                        /*           FORK# 1          */
    int pid=fork();
    if(pid==0) {
                                                                                        /*         STDIN LOOP         */
        close(fd[0]);
        char input[128];
        while(1) {
            int rSize=read(STDIN_FILENO, input, 128);
            if(rSize<1) {
                if(rSize==0) {
                    continue;
                }
                else if(rSize==-1) {
                    printErrorAndExit("Error reading from STDIN", -1);
                }
            }
            if(input[0]=='\n') {
                continue;
            }
            char str[144];
            int n=sprintf(str, "stdin %s", input);
            if(write(fd[1], str, n-1)==-1) {
                printErrorAndExit("Error writing to pipe", -1);
            }
        }
        exit(0);
    }
    else if(pid>0) {
                                                                                        /*           FORK# 2          */
        int pid2=fork();
        if(pid2>0) {
                                                                                        /*       PIPE READ LOOP       */
            close(fd[1]);
            char* arr[ARG_SIZE];
            char buff[128];
            //char buffCpy[128];
            while(1) {
                memset(buff, '\0', 128);
                int size=read(fd[0], buff, 128);
                if(size==-1) {
                    printErrorAndExit("Error reading from pipe", -1);
                }
                if(size==0) {
                    continue;
                }
                //buff[size]='\0';
                if(buff[size-1]=='\n') {
                    buff[size-1]='\0';
                }
                //write(STDOUT_FILENO, buff, size);
                //strcpy(buffCpy, buff, 128);
                int i=0;
                arr[0]=strtok(buff, " ");
                while(arr[i]!=NULL&&i<ARG_SIZE) {
                    i++;
                    arr[i]=strtok(NULL, " ");
                }
                if(i==0) {
                    continue;
                }
                if(strcmp(arr[0], "stdin")==0) {
                    if(i<2) {
                        continue;
                    }
                    /*write(STDOUT_FILENO, arr[0], strlen(arr[0]));
                    write(STDOUT_FILENO, ":", 1);
                    write(STDOUT_FILENO, arr[1], strlen(arr[1]));
                    write(STDOUT_FILENO, ":", 1);*/
                    if(strcmp(arr[1], "exit")==0) {
                        write(STDOUT_FILENO, "Exiting...\n", 11);
                        kill(0, SIGTERM);                    
                        continue;
                    }
                    else if(strcmp(arr[1], "list")==0) {
                        if(i<3) {
                            continue;
                        }
                        else {
                            if(strcmp(arr[2], "-c")==0) {
                                listClients();
                            }
                            else if(strcmp(arr[2], "-p")==0) {
                                listProcesses();
                            }
                        }
                        continue;
                    }
                    else if(strcmp(arr[1], "kill")==0) {
                        if(i==4) {
                            if(strcmp(arr[2], "-p")==0) {
                                if(kill(atoi(arr[3]), SIGTERM)==-1) {
                                    write(STDOUT_FILENO, "could not kill process\n", 23);
                                }
                            }
                            else if(strcmp(arr[2], "-n")==0) {
                                int pd=findProcessPidByName(arr[3]);
                                if(pd!=-1) {
                                    if(kill(pd, SIGTERM)==-1) {
                                        write(STDOUT_FILENO, "could not kill process\n", 23);
                                    }
                                }
                            }
                        }
                        else if(i==3) {
                            if(strcmp(arr[2], "-a")==0) {
                                int num=genocide();
                                char sen[16];
                                int s=sprintf(sen, "killed %d processes\n", num);
                                write(STDOUT_FILENO, sen, s);
                            }
                        }
                    }
                    else if(strcmp(arr[1], "kick")==0) {
                        if(i==3) {
                            int n=findSockByAddr(arr[2]);
                            if(n==-1) {
                                write(STDOUT_FILENO, "Client not found\n", 17);
                                continue;
                            }
                            if(write(n, "disconnect", 10)==-1) {
                                perror("error writing");
                                printf(" %d\n", n);
                            }
                            kill(n, SIGTERM);
                        }
                    }
                    else if(strcmp(arr[1], "msg")==0) {         // Implement switches to broadcast
                        if(i>3) {
                            int n=findSockByAddr(arr[2]);
                            if(n==-1) {
                                write(STDOUT_FILENO, "Client not found\n", 17);
                                continue;
                            }
                            char str[512];
                            strcpy(str, "echo");
                            int j=3;
                            while(arr[j]!=NULL) {
                                strcat(str, " ");
                                strcat(str, arr[j]);
                                j++;
                            }
                            if(write(n, str, strlen(str))==-1) {
                                perror("Error writing");
                                break;
                            }
                        }
                    }
                    else if(strcmp(arr[1], "broadcast")==0) {
                        if(i>2) {
                            char str[512];
                            strcpy(str, "echo");
                            int j=2;
                            while(arr[j]!=NULL) {
                                strcat(str, " ");
                                strcat(str, arr[j]);
                                j++;
                            }
                            broadcast(str, strlen(str));
                        }
                    }
                }
                else {
                    if(strcmp(arr[0], "client")==0) {
                        int sockfd=socket(AF_INET, SOCK_STREAM, 0);
                        struct sockaddr_in server;
                        server.sin_family = AF_INET;
                        struct hostent *hp = gethostbyname(arr[2]);
                        if (hp == 0) {
                            exit(-1);
                        }
                        bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
                        server.sin_port = htons(atoi(arr[1]));
                        if(connect(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
                            perror("connecting socket");
                            exit(-1);
                        }
                        addClient(arr[2], atoi(arr[3]), atoi(arr[4]), sockfd);
                        if(write(cLast->socket, "echo Connection established\n", 28)==-1) {
                            perror("Cannot write");
                        }
                    }
                    else if(strcmp(arr[0], "proc")==0) {
                        addProcess(arr[1], arr[2], atoi(arr[3]), atoi(arr[4]), atoi(arr[5]));
                    }
                    else if(strcmp(arr[0], "term")==0) {
                        int csock=findSockByAddr(arr[2]);
                        char warr[80];
                        int n=sprintf(warr, "echo %s exited with state %d\n", arr[1], atoi(arr[3]));
                        if(csock!=-1) {
                            if(write(cLast->socket, warr, n)==-1) {
                                perror("write");
                            }
                        }
                        int id=atoi(arr[4]);
                        setProcessState(id, atoi(arr[3]));
                        /*char tim[32];
                        strcpy(tim, arr[5]);*/
                        /*time ( &rawtime );
                        timeinfo = localtime (&rawtime);*/
                        stampEndTime(id, asctime(timeinfo));
                        //removeProcess(atoi(arr[4]));
                    }
                }
            }
        }
        else if(pid2==0){
            if(signal(SIGCHLD, handler)==SIG_ERR) {
                printErrorAndExit("Error registering SIGCHLD handler", -1);
            }
            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = INADDR_ANY;
            server.sin_port = 0;////////////////////////// 0
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                printErrorAndExit("Error opening socket", -1);
            }
            if (bind(sock, (struct sockaddr *) &server, sizeof(server))==-1) {
                printErrorAndExit("Error binding socket", -1);
            }
            int length = sizeof(server);
            if (getsockname(sock, (struct sockaddr *) &server, (socklen_t*) &length)) {
                printErrorAndExit("Error getsockname", -1);
            }
            char pst[15];
            int k=sprintf(pst, "Port: %d\n", ntohs(server.sin_port));
            write(STDOUT_FILENO, pst, k);
            
            listen(sock, 5);
            while(1) {
                                                                                            /*         ACCEPT LOOP        */
                struct sockaddr_in client;
                int cSize=sizeof(client);
                int clientSock = accept(sock, (struct sockaddr*)&client, &cSize);
                if(clientSock==-1) {
                    printErrorAndExit("Error accepting connection", -1);
                }
                                                                                            /*           FORK# 3          */
                int pid3=fork();
                if(pid3==0) {
                                                                                            /*      CLIENT ENTERTAINER     */
                    char strport[64];
                    int size=read(clientSock, strport, 64);
                    if(size==-1) {
                        printErrorAndExit("Read", -1);
                    }
                    char clAddr[INET_ADDRSTRLEN+6];
                    inet_ntop(AF_INET, &client.sin_addr, clAddr, INET_ADDRSTRLEN);
                    sprintf(strport, "%s %s %d %d", strport, clAddr, client.sin_port, getpid());
                    if(write(fd[1], strport , strlen(strport))==-1) {
                        printErrorAndExit("Write", -1);
                    }
                    sprintf(clAddr, "%s:%d", clAddr, client.sin_port);
                    //printf("New client @ %s\n", clAddr);
                    char msg[READ_SIZE];
                    char* arr[ARG_SIZE];
                    int rval;
                    while(1) {
                                                                                            /*      CLIENT READ LOOP     */
                        rval=read(clientSock, msg, READ_SIZE);
                        if(rval<1) {
                            if(rval==0) {
                                printf("Client disconnected %s\n", clAddr);
                                exit(0);
                            }
                            else if(rval==-1) {
                                printErrorAndExit("Error reading socket", -1);
                            }
                        }
                        if(msg[0]=='\n') {
                            continue;
                        }
                        msg[rval-1]='\0';

                        
                                                                                            /*           PARSING          */
                        int i=0;
                        arr[0]=strtok(msg, " ");
                        while(arr[i]!=NULL&&i<ARG_SIZE) {
                            i++;
                            arr[i]=strtok(NULL, " ");
                        }
                        if(i==0) {
                            continue;
                        }
                        if(strcmp(arr[0], "run")==0) {
                            if(i<2) {
                                write(sock, "No target given\n", 16);
                                continue;
                            }
                            
                                                                                            /*           FORK# 4          */
                            int fd2[2];
                            pipe2(fd2, O_CLOEXEC);
                            int pid4=fork();
                            if(pid4==0) {
                                if(execvp(arr[1], &arr[1])==-1) {
                                    perror("Exec error: ");
                                    if(write(fd2[1], "1", 1)==-1) {
                                        perror("write");
                                    }
                                    char tmp[13];
                                    int rn=sprintf(tmp, "error %d", getpid());
                                    if(write(fd[1], tmp, rn)==-1) { // Use the exit status or a seperate bit?? Or custom signal TODO
                                        perror("Probs writing to pipe");
                                    }
                                    exit(-1);
                                }
                            }
                            else {
                                close(fd2[1]);
                                char dump[2];
                                int retV=read(fd2[0], dump, 2);
                                if(retV==0) {
                                    int parent=getppid();
                                    addProcess(clAddr, arr[1], parent, pid4, 1);
                                    char tmp[128];
                                    int n=sprintf(tmp, "proc %s %s %d %d %d", clAddr, arr[1], parent, pid4, 1);
                                    write(fd[1], tmp, n);
                                }
                                else {
                                    char tmp[64];
                                    int sz=sprintf(tmp, "Could not run %s\n", arr[1]);
                                    write(clientSock, tmp, sz);
                                }
                            }
                        }
                        else if(strcmp(arr[0], "list")==0) {
                            if(i>1) {
                                if(strcmp(arr[1], "-a")==0) {
                                    struct Process *p=head;
                                    while(p!=NULL) {
                                        char buff[128];
                                        int n=sprintf(buff, "%s %d %s %d %s %s\n", p->addr, p->pId, p->name, p->state, p->start_time, p->end_time);
                                        if((write(clientSock, buff, n))==-1) {
                                            perror("Write");
                                            break;
                                        }
                                        p=p->next;
                                    }
                                }
                            }
                            else {
                                struct Process *p=head;
                                while(p!=NULL) {
                                    if(p->state==1) {
                                        char buff[128];
                                        int n=sprintf(buff, "%s %d %s %d %s %s\n", p->addr, p->pId, p->name, p->state, p->start_time, p->end_time);
                                        if((write(clientSock, buff, n))==-1) {
                                            perror("Write");
                                            break;
                                        }
                                    }
                                    p=p->next;
                                }
                            }
                        }
                        else if(strcmp(arr[0], "add")==0) {
                            if(i<3) {
                                write(clientSock, "Not enough arguments for add\n", 29);
                                continue;
                            }
                            int sum = 0;
                            int o=1;
                            while(arr[o]!=NULL) {
                                sum=sum+atoi(arr[o]);
                                o++;
                            }
                            char buff[16];
                            int n=sprintf(buff, "%d\n", sum);
                            write(clientSock, buff, n);
                        }
                        else if(strcmp(arr[0], "sub")==0) {
                            if(i<3) {
                                write(clientSock, "Not enough arguments for sub\n", 29);
                                continue;
                            }
                            char buff[16];
                            int n=sprintf(buff, "%d\n", atoi(arr[1])-atoi(arr[2]));
                            write(clientSock, buff, n);
                        }
                        else if(strcmp(arr[0], "mult")==0) {
                            if(i<3) {
                                write(clientSock, "Not enough arguments for mult\n", 30);
                                continue;
                            }
                            int prod = 1;
                            int o=1;
                            while(arr[o]!=NULL) {
                                prod=prod*atoi(arr[o]);
                                o++;
                            }
                            char buff[16];
                            int n=sprintf(buff, "%d\n", prod);
                            write(clientSock, buff, n);
                        }
                        else if(strcmp(arr[0], "div")==0) {
                            if(i<3) {
                                write(clientSock, "Not enough arguments for div\n", 29);
                                continue;
                            }
                            if(atoi(arr[2])==0) {
                                write(clientSock, "Cannot divide be zero\n", 22);
                                continue;
                            }
                            char buff[16];
                            int n=sprintf(buff, "%d\n", atoi(arr[1])/atoi(arr[2]));
                            write(clientSock, buff, n);
                        }
                        else if(strcmp(arr[0], "kill")==0) {
                            if(i>2) {
                                if(strcmp(arr[1], "-p")==0) {
                                    int a=atoi(arr[2]);
                                    if(a>0) {
                                        if(kill(a, SIGTERM)==-1) {
                                            write(clientSock, "could not kill process\n", 23);
                                        }
                                    }
                                }
                                else if(strcmp(arr[1], "-n")==0) {
                                    int p=findProcessPidByName(arr[2]);
                                    if(p!=-1) {
                                        if(kill(p, SIGTERM)==-1) {
                                            write(clientSock, "could not kill process\n", 23);
                                        }
                                    }
                                    else {
                                        write(clientSock, "process not found\n", 18);
                                    }
                                }
                            }
                            else if(i>1) {
                                if(strcmp(arr[1], "-a")==0) {
                                    int num=genocide();
                                    char buff[16];
                                    int s=sprintf(buff, "killed %d processes\n", num);
                                    write(clientSock, buff, s);
                                }
                            }
                            else {
                                write(clientSock, "Not enough arguments for kill\n", 30);
                                continue;
                            }
                        }
                        else {
                            write(STDOUT_FILENO, "Unrecognized command\n", 21);
                        }
                    }
                }
            }
        }
    }
}

