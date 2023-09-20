/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <stdlib.h>
#define original_staticx
// cgi : common gateway interface
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetread_requesthdrsype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

// 응답을 해주는 함수
void doit(int fd)
{
  int is_static;      // 정적파일인지 아닌지를 판단해주기 위한 변수
  
  struct stat sbuf;   // 파일에 대한 정보를 가지고 있는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);              // rio(robust I/O (Rio)) 초기화
  Rio_readlineb(&rio, buf, MAXLINE);    // buf에서 client request 읽어들이기
  printf("Request headers:\n");
  printf("%s", buf);                    // request header 출력
  /* buf에 있는 데이터(header)를 method, uri, version에 담기
   * method : GET , uri : test.mp4 , version : HTTP1.1
   */
  sscanf(buf, "%s %s %s", method, uri, version);

  // method가 GET이 아니라면 error message 출력
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  // if (strcasecmp(method, "GET")) {
  //   clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
  //   return;
  // }
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);
  // filename에 맞는 정보 조회를 하지 못했으면 error message 출력 (return 0 if success else -1)
  if (stat(filename, &sbuf) < 0) {
      clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
      return;
  }
  // request file이 static contents이면 실행
  if (is_static) {
    // file이 정규파일이 아니거나 사용자 읽기가 안되면 error message 출력
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // response static file
    serve_static(fd, filename, sbuf.st_size, method);
  }
  // request file이 dynamic contents이면 실행
  else {
    // file이 정규파일이 아니거나 사용자 읽기가 안되면 error message 출력
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
      "Tiny couldn't run the CGI program");
      return;
  }
  // response dynamic files
    serve_dynamic(fd, filename, cgiargs, method);
  }
}

// error 발생 시, client에게 보내기 위한 response (error message)
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];
  // response body 쓰기 (HTML 형식)
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  //response 쓰기
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);        // 버전, 에러번호, 상태메시지
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));                          // body 입력
}

// request header를 읽기 위한 함수
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  // 빈 텍스트 줄이 아닐 때까지 읽기 (못 읽어온 헤더 읽기)
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

// uri 분해해서 쿼리를 담는다
// uri parsing을 하여 static을 request하면 0, dynamic을 request하면 1반환
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

// cgi~ : 쿼리 저장
  // parsing 결과, static file request인 경우 (uri에 cgi-bin이 포함이 되어 있지 않으면)
  if (!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    // request에서 특정 static contents를 요구하지 않은 경우 (home page 반환) - 허전하니까...
    if (uri[strlen(uri)-1] == '/') {
      strcat(filename, "home.html");
    }
    return 1;
  }
  // parsing 결과, dynamic file request인 경우
  else {
    // uri부분에서 file name과 args를 구분하는 ?위치 찾기
    ptr = index(uri, '?');
    // ?가 있으면
    if (ptr) {
      //cgiargs에 인자 넣어주기
      strcpy(cgiargs, ptr+1);
      // 포인터 ptr은 null처리
      *ptr = '\0';
    }
    // ?가 없으면
    else {
      strcpy(cgiargs, "");
    }
    // filename에 uri담기
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

#ifdef original_static
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

#else
// HEAD method 처리를 위한 인자 추가
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  if(strcasecmp(method, "GET") == 0) {
    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    // solved problem 11.9
    srcp = malloc(filesize);
    Rio_readn(srcfd, srcp, filesize);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    // Munmap(srcp, filesize);
    free(srcp);
  }
}
#endif
  /* * get_filetype - Derive file type from filename*/
  void get_filetype(char *filename, char *filetype)
  {
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

// 동적인 서빙(?) 일처리를 해주다!
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

// syscall -> 자식프로세서 생성 = return 0

  if (Fork() == 0) { /* Child */
  /* Real server would set all CGI vars here */
  setenv("QUERY_STRING", cgiargs, 1);
  // method를 cgi-bin/adder.c에 넘겨주기 위해 환경변수 set
  setenv("REQUEST_METHOD", method, 1);

  // 표준출력을 터미널이 아닌 서버소켓 fd로 redirect한다. (서버fd는 클과 연결되어있음) >> adder 의 print부분 (printf가 바로 클라이언트로...)
  Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
  // Execve : sys call 새로운 프로그램 실행 -> 자식을 만들어서 실행 (filname : adder 의 실행파일 경로(위치))
  Execve(filename, emptylist, environ); /* Run CGI program */
  }
  // 자식이 없어질 때 까지 멈춰있음
  Wait(NULL); /* Parent waits for and reaps child */
}

/* 입력 ./tiny 8080 / argc = 2 / argv[0] = tiny, argv[1] = */
int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;                  // client socket's length
  struct sockaddr_storage clientaddr;   // client socket's addr

  if (argc != 2) {                      // port number를 입력안했을 경우 error
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  /* Open_listenfd 함수를 호출해서 듣기 소켓을 오픈
   * @param argv[1]  포트번호
   * @return linstendfd - 요청받을 준비가 된 듣기 식별자
   */
  listenfd = Open_listenfd(argv[1]);

  // 무한 서버 루프 실행
  while (1) {
    clientlen = sizeof(clientaddr);     //accpet 함수 인자에 넣기위한 주소길이를 계산

    /* 반복적으로 연결 요청을 접수 */
    /* Accept 함수 인자 : 1. 듣기 식별자, 2. 소켓주소 구조체의 주소, 3. 주소(소켓구조체)의 길이 */
    connfd = Accept(listenfd, (SA *)&clientaddr,  &clientlen);  // SA: type of struct socketaddr

    /* Getnameinfo : client socket 주소 구조체에서 host과 port 번호를 스트링으로 변환
     * Getaddrinfo는 호스트 이름: 호스트 주소(www.~.com/127.xxx.~), 서비스 이름: 포트 번호(http/80) 의 스트링 표시를 소켓 주소 구조체(addrinfo)로 변환
    */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

    /* 트랜젝션을 수행 */
    doit(connfd); // line:netp:tiny:doit
    /* 트랜잭션이 수행된 후 자신 쪽의 연결 끝 (소켓) 을 닫는다. */
    Close(connfd); // line:netp:tiny:close
  }
}