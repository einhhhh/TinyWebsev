#include "csapp.h"


/*
typedef struct {
    int rio_fd;                // Descriptor for this internal buf 
    int rio_cnt;               // Unread bytes in internal buf 
    char *rio_bufptr;          // Next unread byte in internal buf
    char rio_buf[RIO_BUFSIZE]; // Internal buffer
} rio_t;
*/
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/*
all of the following steps are implemented in a loop
step1: create the listening file descriptor with the user specified port
step2: invoke Accept() to create a new connecting file descriptor to get the request
step3: invoke doit() to handle the conneciton
step4: dont forget to close the connecting file descriptor explicitly
*/

int main(int argc, char **argv){
	int listenfd, connfd;
	// MAXLINE is defined in csapp.h, the value of it is 8192
	char hostname[MAXLINE], port[MAXLINE];
	// socklen_t is a kind of value type, it can be seem as int. Used by the third arg of accept()
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;

	if (argc != 2){
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	listenfd = Open_listenfd(atoi(argv[1]));

	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		doit(connfd);
		Close(connfd);
	}


}

void doit(int fd){
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	// Read the request line and headers
	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);
	printf("Request headers:\n");
	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);
	if(strcasecmp(method, "GET")){
		clienterror(fd, method, "501", "Not implemented", 
			"Hn13 does not implemented this method");
		return ;
	}
	read_requesthdrs(&rio);

	// Parse URI from GET requset
	is_static = parse_uri(uri, filename, cgiargs);
	// user stat() to find if this file exsit or not.
	// as well as the status of this file
	if(stat(filename, &sbuf)<0){
		clienterror(fd, filename, "404", "Not Found",
			"Hn13 couldnt find this file");
		return ;
	}

	if(is_static){
		// judge if this file is a regular file or not
		// privilege judging
		if(!(S_ISREG(sbuf.st_mode))||!(S_IRUSR & sbuf.st_mode)){
			clienterror(fd, filename, "403", "Forbidden",
			"Hn13 couldnt read the file");
		return ;
		}
		serve_static(fd, filename, sbuf.st_size);
	}else{
		if(!(S_ISREG(sbuf.st_mode))||!(S_IRUSR & sbuf.st_mode)){
			clienterror(fd, filename, "403", "Forbidden",
			"Hn13 couldnt read the file");
			return ;
		}
		serve_dynamic(fd, filename, cgiargs);
	}

}
// constructe the HTTP response body and response header
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg){
	
	char buf[MAXLINE], body[MAXLINE];

	// HTTP response body
	sprintf(body, "<html><title>Hn13 Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Hn13 Web server</em>\r\n", body);

	// HTTP response header
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}



void read_requesthdrs(rio_t *rp){

	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);

	while(strcmp(buf, "\r\n")){
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return ;
}

int parse_uri(char *uri, char *filename, char *cgiargs){

	char *ptr;

	if(!strstr(uri, "cgi-bin")){
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if(uri[strlen(uri)-1]=='/')
			strcat(filename, "home.html");
		return 1;
	}else{
		// get address of ? 
		ptr = index(uri, '?');
		if(ptr){
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		}else
			strcpy(cgiargs, "");

		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}

}


void serve_static(int fd, char *filename, int filesize){

	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXLINE];

	// constructe response headers to clinet
	get_filetype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Hn13 Web Server \r\n", buf);
	sprintf(buf, "%sConnection: Close\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));
	printf("Response headers: \n");
	printf("%s",buf);

	// send the response body
	srcfd = Open(filename, O_RDONLY, 0);
	// map the srcfd's pre filesiOze bytes data to the virsual memory space.
	srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	Close(srcfd);
	Rio_writen(fd, srcp, filesize);
	Munmap(srcp, filesize);

}

void get_filetype(char *filename, char *filetype){

	if(strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if(strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if(strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if(strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpg");
	else 
		strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs){

	char buf[MAXLINE], *emptylist[] = { NULL };

	// the first part of HTTP response
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Hn13 Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if(Fork() == 0){
		// set the environment value
		setenv("QUERY_STRING", cgiargs, 1);
		//use dup2 to redirect the file descriptor
		Dup2(fd, STDOUT_FILENO);
		Execve(filename, emptylist, environ);
	}
	Wait(NULL);
}