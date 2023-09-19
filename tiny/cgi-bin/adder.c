/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"
int main(void)
{
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    // buf 에서 '&'주소 반환
    p = strchr(buf, '&');
    *p = '\0';
    // &는 null 이므로 그 전까지 읽는다.
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);

    // 문자열을 정수로 바꿪무
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }

  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
          content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n"); // \r\n 헤더의 끝
  printf("%s", content);
  // 표준출력버퍼 정리
  fflush(stdout);

  exit(0);
}
/* $end adder */
