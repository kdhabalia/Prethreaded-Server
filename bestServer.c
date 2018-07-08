#include "bestServer.h"
#include "sbuf.h"
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#define NTHREADS 1000
#define SBUFSIZE 3000

sbuf_t sbuf;

void* thread(void* vargp);

int main(int argc, char **argv)
{
    int i, listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);

    sbuf_init(&sbuf, SBUFSIZE);
    for(i = 0; i < NTHREADS; i++) {
      Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
		  clientlen = sizeof(clientaddr);
		  connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
			sbuf_insert(&sbuf, connfd);
    }
}


void* thread(void* vargp)
{
  Pthread_detach(pthread_self());
  while(1) {
    int connfd = sbuf_remove(&sbuf);
    doit(connfd);
    Close(connfd);
  }
}


void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;


    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    read_requesthdrs(&rio);

    is_static = parse_uri(uri, filename, cgiargs);
    //if (stat(filename, &sbuf) < 0) {
		//clienterror(fd, filename, "404", "Not found",
		//    		"Tiny couldn't find this file");
		//return;
    //}

    if (is_static) { /* Serve static content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
		    clienterror(fd, filename, "403", "Forbidden",
				"Tiny couldn't read the file");
		    return;
		}
		serve_static(fd, filename, sbuf.st_size);
	} else { /* Serve dynamic content */
		//if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			//clienterror(fd, filename, "403", "Forbidden",
			//	"Tiny couldn't run the CGI program");
			//return;
	  //}
		serve_dynamic(fd, filename, cgiargs);
    }
}



void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
    }
    return;
}



int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    /*if (!strstr(uri, "cgi-bin")) {
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if (uri[strlen(uri)-1] == '/')	strcat(filename, "home.html");
		return 1;
    } else {*/
		ptr = index(uri, '?');
		if (ptr) {
	    	strcpy(cgiargs, ptr+1);
	    	*ptr = '\0';
		} else strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;

}



void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}



void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
    else
		strcpy(filetype, "text/plain");
}


void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };
    char* p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1=0, n2=0;

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    char* error;
    void *handle;
    int (*math) (int, int, int);

    int add = (strcmp(filename, "./cgi-bin/adder"));
    int sub = (strcmp(filename, "./cgi-bin/subtract"));
    int mult = (strcmp(filename, "./cgi-bin/multiply"));
    int div = (strcmp(filename, "./cgi-bin/divide"));

    if(add && sub && mult && div) {
      sprintf(buf, "Function not implemented\r\n");
      Rio_writen(fd, buf, strlen(buf));
      return;
    }

	  p = strchr(cgiargs, '&');
	  *p = '\0';
	  strcpy(arg1, cgiargs);
	  strcpy(arg2, p+1);
	  n1 = atoi(arg1);
	  n2 = atoi(arg2);

    strcat(filename, ".so");
    handle = dlopen(filename, RTLD_LAZY);
    if(!handle) {
      sprintf(buf, "%s\r\n", dlerror());
      Rio_writen(fd, buf, strlen(buf));
      return;
    }

    if(!add) math = dlsym(handle, "adder");
    else if(!sub) math = dlsym(handle, "subtract");
    else if(!mult) math = dlsym(handle, "multiply");
    else if(!div) math = dlsym(handle, "divide");

    if((error = dlerror()) != NULL) {
      sprintf(buf, "%s\r\n", dlerror());
      Rio_writen(fd, buf, strlen(buf));
      return;
    }

    //Dup2(fd, STDOUT_FILENO);
    math(n1, n2, fd);

    if(dlclose(handle) < 0) {
      sprintf(buf, "%s\r\n", dlerror());
      Rio_writen(fd, buf, strlen(buf));
      return;
    }
    return;

		       /* Redirect stdout to client */ //line:netp:servedynamic:dup2
}


void clienterror(int fd, char *cause, char *errnum,
		 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */
