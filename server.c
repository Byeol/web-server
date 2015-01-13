#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MESGSIZE 4096
#define PORT 32000

#define BASIC_RESPONSE_HEADER "HTTP/1.1 %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n"

#define STATUS_CODE_200 "200 OK"
#define STATUS_CODE_404 "404 Not Found"

#define STATIC_RESOURCE_PATH_404 "error_404.html"

struct {
	char * ext;
	char * mimetype;
} extensions [] = {
	{ "jpg",	"image/jpg" },
	{ "png",	"image/png" },
	{ "html",	"text/html" },
	{ 0, 0 }
};

long get_filesize(int fd)
{
	long len;
	len = lseek(fd, (off_t) 0, SEEK_END);
	lseek(fd, (off_t) 0, SEEK_SET);

	return len;
}

char * get_mimetype(char * filename, int length)
{
	int ext_len;

	for(int i = 0; extensions[i].ext != 0; i++) {
		ext_len = strlen(extensions[i].ext);
		if (!strncmp(&filename[length - ext_len], extensions[i].ext, ext_len)) {
			return extensions[i].mimetype;
		}
	}

	return NULL;
}

int main(int argc, char **argv)
{
	int filefd, listenfd, connfd, n;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t clilen;
	char mesg[MESGSIZE];
	char buf[BUFSIZ];

	char * req_filename;
	long req_filesize;
	char * req_mimetype;
	char * res_statuscode;

	int port = PORT;

	listenfd = socket(AF_INET, SOCK_STREAM,0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, 1024);

	for(int i = 0; i < 100; i++) {
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		printf("\nconnected (%d)\n", connfd);

		if (connfd > 0) {
			n = recv(connfd, mesg, MESGSIZE, 0);
			mesg[n] = 0;

			// Request Message
			printf("=====\n%s=====\n", mesg);

			// according to rfc 2616, the method is case-sensitive.
			if (strncmp(mesg, "GET ", 4)) {
				printf("Only GET method is supported");
			}

			// ignore other headers
			for(int j = 4; j < MESGSIZE; i++) {
				if (mesg[j] == ' ') {
					mesg[j] = 0;
					break;
				}
			}

			if (!strncmp(mesg, "GET /\0", 6)) {
				strcpy(mesg, "GET /index.html");
			}

			req_filename = &mesg[5];
			printf("Request %s\n", req_filename);

			if ( (filefd = open(req_filename, O_RDONLY)) == -1) {
				// 404 Not Found
				printf("404 Not Found\n");

				filefd = open(STATIC_RESOURCE_PATH_404, O_RDONLY);
				req_filename = STATIC_RESOURCE_PATH_404;

				res_statuscode = STATUS_CODE_404;
				req_filesize = get_filesize(filefd);
				req_mimetype = get_mimetype(req_filename, strlen(req_filename));
			} else {
				// 200 OK
				printf("200 OK\n");

				res_statuscode = STATUS_CODE_200;
				req_filesize = get_filesize(filefd);
				req_mimetype = get_mimetype(req_filename, strlen(req_filename));
			}

			sprintf(buf, BASIC_RESPONSE_HEADER, res_statuscode, req_filesize, req_mimetype);
			send(connfd, buf, strlen(buf), 0);

			while ((n = read(filefd, buf, BUFSIZ)) > 0) {
				send(connfd, buf, n, 0);
			}

			close(filefd);
			close(connfd);
		}
	}
}
