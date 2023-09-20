#include "csapp.h"

void echo(int fd);

void echo(int fd) {
  size_t n;
  char buf[MAXLINE];
  rio_t rio;
  printf("tiny echo\n");
  Rio_readinitb(&rio, fd);
  while ( (n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
    printf("server received %d bytes\n", (int)n);
    Rio_writen(fd, buf, n);
  }
}