#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <cstdio>
#include <errno.h>

using namespace std;



void send_fd(int socket, int fd)  // send fd by socket
{
    struct msghdr msg = { 0 };
    char buf[CMSG_SPACE(sizeof(fd))];
    memset(buf, '\0', sizeof(buf));
    struct iovec io = {};

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

    *((int *) CMSG_DATA(cmsg)) = fd;

    msg.msg_controllen = cmsg->cmsg_len;

    if (sendmsg(socket, &msg, 0) < 0)
     {}
}

int receive_fd(int socket)  // receive fd from socket
{
    struct msghdr msg = {0};
    char m_buffer[256];
    struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    char c_buffer[256];
    msg.msg_control = c_buffer;
    msg.msg_controllen = sizeof(c_buffer);

    if (recvmsg(socket, &msg, 0) < 0)
        {}

    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);

    unsigned char * data = CMSG_DATA(cmsg);
    int fd = *((int*) data);
    return fd;
}

void headers(int client, int size, char *fext, int httpcode) {
	char buf[1024];
	char strsize[20];
	sprintf(strsize, "%d", size);
	cout<<"httpcode = "<<httpcode<<"\n";
	if (httpcode == 200) {
		strcpy(buf, "HTTP/1.0 200 OK\r\n");
	}
	else if (httpcode == 404) {
		strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
	}
	else if (httpcode== 403) {
		strcpy(buf, "HTTP/1.0 403 Access Forbidden\r\n");
		send(client, buf, strlen(buf), 0);
		strcpy(buf, "\r\n");
		close(client);		
		//send(client, buf, strlen(buf), 0);
		//close(client);
		return;
	}
	else
	{
		strcpy(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	}
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "Connection: keep-alive\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "Content-length: ");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, strsize);
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "simple-server");
	send(client, buf, strlen(buf), 0);
	if (strstr(fext, "txt"))
	{
		sprintf(buf, "Content-Type: text/html\r\n");
	}
	else if (strstr(fext, "jpeg")||strstr(fext, "jpg"))
	{
		sprintf(buf, "Content-Type: image/jpeg\r\n");
	}
	else if (strstr(fext, "pdf"))
	{
		sprintf(buf, "Content-Type: application/pdf\r\n");
	}
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}


void parseFileName(char *line, char **filepath, size_t *len) {
	char *start = NULL;
	while ((*line) != '/') line++;
	start = line + 1;
	while ((*line) != ' ') line++;
	(*len) = line - start;
	*filepath = (char*)malloc(*len + 1);
	*filepath = strncpy(*filepath, start, *len);
	(*filepath)[*len] = '\0';
	printf("%s \n", *filepath);
}

char* gfe(char* fullfilename)
{
   int size, index;
   size = index = 0;

   while(fullfilename[size] != '\0') {
      if(fullfilename[size] == '.') {
         index = size;
      }
       size ++; 
   }
   if(size && index) {
      return fullfilename + index + 1;
   }
      return NULL;
}

struct pStruct
{
pid_t proc;
int stat;
};
const int n = 3;
pStruct masProc[n];

int freeProc()
{
	for (int i = 0; i < n; i++){
		
		if (masProc[i].stat == 0)
		{
			
			return masProc[i].proc;
		}	
	}
	return -1;
}

int procById(int id)
{	
	for (int i = 0; i < n; i++)
	{
		if (masProc[i].proc == id)
		{		
			return i;
		}
	}
	return -1;
}

int pipefd[n][2];

void signalHandler(int sig, siginfo_t *siginfo, void *context)
{
	masProc[procById(siginfo->si_pid)].stat = -1;
	cout<<"doneid = "<<siginfo->si_pid<<"\n";
}

int go = 0;
void goSet(int sig)
{
	go = 1;
}

void sigact()
{
	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	act.sa_sigaction = &signalHandler;
	act.sa_flags = SA_SIGINFO | SA_RESTART;
	if (!(sigaction(SIGUSR1, &act, NULL)<0))
	cout<<"sigaction complete"<<"\n";
	signal(SIGUSR2, goSet);
}




int main() {
	int ld = 0;
	int res = 0;
	int cd = 0;
	int filesize = 0;
	const int backlog = 10;
	struct sockaddr_in saddr;
	struct sockaddr_in caddr;
	char *line = NULL;
	size_t len = 0;
	char *filepath = NULL;
	char *fext = NULL;
	size_t filepath_len = 0;
	int empty_str_count = 0;
	socklen_t size_saddr;
	socklen_t size_caddr;
	FILE *fd;
	FILE *file;
	int read_size;
	int ttt = 248;
	ld = socket(AF_INET, SOCK_STREAM, 0);
	if (ld == -1) {
		printf("listener create error \n");
	}
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8080);
	saddr.sin_addr.s_addr = INADDR_ANY;
	printf("binding \n");
	res = bind(ld, (struct sockaddr *)&saddr, sizeof(saddr));
	if (res == -1) {
		printf("bind ERROR %d\n", errno);
	}
	printf("listening \n");
	pid_t chpid;
	pid_t ppid = getpid();
	sigact();
	
	for (int i = 0; i<n; i++) {
		//pipe(pipefd[i]);
		if (socketpair(AF_UNIX, SOCK_DGRAM, 0, pipefd[i]) != 0)
        		printf("%s","Failed to create Unix-domain socket pair\n");
	}
	for (int i = 0; i<n; i++) {
		chpid = fork();
		masProc[i].stat = 0;
		if (chpid == 0)
		{
			masProc[i].proc = getpid(); 	
			break;
		}
		masProc[i].proc = chpid;
	}
	int t = 0;
	
	while (1)
	{
		if (chpid==0) break;
		res = listen(ld, backlog);
		if (res == -1) {
			printf("listen error %d\n", errno);
			return 1;
		}
				
		cd = accept(ld, (struct sockaddr *)&caddr, &size_caddr);
		if (cd == -1) {
				printf("accept ERROR %d\n", errno);
		}
		
		for (int i = 0; i<n; i++)
		cout<<"id="<<masProc[i].proc<<" stat= "<<masProc[i].stat<<"\n";
			
		int free = freeProc();
		cout<<"accept n= "<<++t<<" cd="<<cd<<"freeproc = "<<free<<"\n";
			
		if (free < 0){
			char c[] = {0};
			headers(cd, 0, NULL, 403);
			continue;
		}	
		close(pipefd[procById(free)][1]);
		send_fd(pipefd[procById(free)][0], cd);
		close(pipefd[procById(free)][0]);
		cout<<"sendfd"<<"\n";
		masProc[procById(free)].stat = 1;		
		kill(free, SIGUSR2);
		int newchpid = 0;		
		for (int i = 0; i<n; i++) {
			if (masProc[i].stat<0)
			{
				cout<<"restart = "<<masProc[i].proc<<"\n";
				masProc[i].stat = 0;
				if (socketpair(AF_UNIX, SOCK_DGRAM, 0, pipefd[i]) != 0)
        					printf("%s","Failed to create Unix-domain socket pair\n");				
				newchpid = fork();
				masProc[i].stat = 0;				
				if (newchpid>0) {
					masProc[i].proc = newchpid;
				}
				if (newchpid == 0){ 
					masProc[i].proc = getpid();
					chpid = 0;
					break;
				}
			}
		}
	}
	while (1) {		
			while (!go)
			{

			}
			go = 0;
			close(pipefd[procById(getpid())][0]);
			cout<<"ready to go"<<"\n";
			
			cd = receive_fd(pipefd[procById(getpid())][1]);

			cout<<"im going! "<<getpid()<<"cd="<<cd<<"\n";			
			printf("client in %d descriptor. Client addr is %d \n", cd, caddr.sin_addr.s_addr);
			fd = fdopen(cd, "r");
			if (fd == NULL) {
				printf("error open client descriptor as file \n");
			}
			while ((res = getline(&line, &len, fd)) != -1) {
				//cout<<line<<"\n";				
				if (strstr(line, "GET")) {
					parseFileName(line, &filepath, &filepath_len);
				}
				if (strcmp(line, "\r\n") == 0) {
					empty_str_count++;
				}
				else {
					empty_str_count = 0;
				}
				
				if (empty_str_count == 1) {
					break;
				}
				printf("%s", line);
			}
			printf("%d is going to work \n", getpid());
			cout<<"strlength=="<<strlen(filepath)<<"\n";
			cout<<"gfe=="<<gfe(filepath)<<"\n";		
			printf("open %s \n", filepath);
			file = fopen(filepath, "r");
			if (file == NULL) {
				printf("404 File Not Found \n");
				headers(cd, 0, gfe(filepath), 404);
			}
			else {
				fseek(file, 0L, SEEK_END);
				filesize = ftell(file);
				fseek(file, 0L, SEEK_SET);
				headers(cd, filesize, gfe(filepath), 200);
				cout<<"filesize "<<filesize<<"\n";
				if (strstr(gfe(filepath), "txt"))
				{
				while (getline(&line, &len, file) != -1) {
					res = send(cd, line, len, 0);
					if (res == -1) {
						printf("send error \n");
					}
				}
				cout<<"----------------------"<<"\n";
				while (1) {}
				}
				if (strstr(gfe(filepath), "jpeg")||strstr(gfe(filepath), "jpg"))
				{
					int c = 0;
					char send_buffer[1024];					
					while(!feof(file)) {	      				
					read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, file);
					send(cd, send_buffer, sizeof(send_buffer)-1,0);  
	      				}
					cout<<"----------------------"<<"\n";
					while (1) {}
				}
				if (strstr(gfe(filepath), "pdf"))
				{
					char send_buffer[1024];
					while(!feof(file)) {
	      				read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, file);
	       				send(cd, send_buffer, sizeof(send_buffer)-1,0);  
	      				}
					cout<<"----------------------"<<"\n";
					while (1) {}
			    	}
			close(pipefd[procById(getpid())][1]);
			close(cd);						
			kill(ppid, SIGUSR1);
			kill(getpid(),SIGINT);
			}		
	}
	return 0;
}



