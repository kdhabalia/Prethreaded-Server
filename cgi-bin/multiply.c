/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
int adder(int n1, int n2, int fd) {
    char* content[MAXLINE], headers[MAXLINE];
    sleep(1);
    /* Make the response body */
    sprintf(content, "Welcome to multiply.com: ");
    sprintf(content, "%sTHE Internet multiplication portal.\r\n<p>", content);
    sprintf(content, "%sThe answer is: %d * %d = %d\r\n<p>",
	    content, n1, n2, n1 * n2);
    sprintf(content, "%sThanks for visiting!\r\n", content);
    sprintf(headers, "Content-length: %d\r\n", (int)strlen(content));
    sprintf(headers, "%sContent-type: text/html\r\n\r\n", headers);
    /* Generate the HTTP response */
    write(fd, headers, strlen(headers));
    write(fd, content, strlen(content));
    return 1;
}
/* $end adder */
