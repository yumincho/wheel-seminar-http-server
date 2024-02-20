#include "http.h"
#include "util.h"

// 추가적으로 필요한 헤더 파일이 있다면 여기에 추가해 주세요.
#include <stdio.h>
#include <string.h>

#define PORT 8080
#define BACKLOG 10

Database g_Database;
int DoSomething(HttpClient* client, const char* request, char* response);

int main(int argc, char** argv) {
  int port = PORT;
  if (argc == 2) {
    if (sscanf(argv[1], "%d", &port) != 1 || port < 0 || port > 65535) {
      printf("잘못된 포트입니다: %s\n", argv[1]);
      return 1;
    }
  } else if (argc >= 3) {
    printf("사용법: %s [포트]\n", argv[0]);
    return 1;
  }

  if (CreateDatabase(&g_Database) < 0) {
    printf("데이터베이스를 생성하지 못했습니다.\n");
    return 1;
  }

  printf("%d번 포트에서 서버를 엽니다.\n", port);

  HttpServer server;
  if (OpenHttpServer(&server, port, BACKLOG) < 0) {
    printf("서버를 열지 못했습니다.\n");
    return 1;
  }

  while (1) {
    HttpClient* client;
    AcceptHttpClient(&server, &client, &DoSomething);
  }
}

int FindLine(int num, char** splited, char* target) {
  for (int i = 0; i < num; i++) {
    if (strstr(splited[i], target)) {
      return i;
    }
  }
  return -1;
}

int DoSomething(HttpClient* client, const char* request, char* response) {
  /*
          아래에 있는 코드를 삭제하고, 여러분만의 코드를 작성해 주세요!

          request 매개변수에는 클라이언트가 보낸 HTTP 요청이 저장되어 있습니다.
     널 문자를 포함하여 최대 2048바이트까지만 저장됩니다. response 매개변수에는
     여러분이 보낼 HTTP 응답을 저장하면 됩니다. 널 문자를 포함하여 최대
     2048바이트까지만 저장할 수 있습니다. (더 긴 길이의 요청/응답이 필요하다면
     http.h 파일의 BUFFER_SIZE 매크로를 수정하면 됩니다.)

          이 함수의 리턴값의 의미는 다음과 같습니다:
          - 음수: 오류가 발생한 경우입니다. 클라이언트에 HTTP 응답을 보내지 않고
     연결을 끊습니다.
          - 0: 클라이언트에 HTTP 응답을 보내고, 연결을 끊지 않고 대기합니다.
          - 양수: 클라이언트에 HTTP 응답을 보내고, 연결을 끊습니다.
  */

  char** splitedRequest;
  int numOfRequestLines = SplitString(request, "\n", &splitedRequest);

  char** splitedRequestByBlankLine;
  SplitString(request, "\r\n\r\n", &splitedRequestByBlankLine);
  char* body = splitedRequestByBlankLine[1];

  char** startLine;
  SplitString(splitedRequest[0], " ", &startLine);

  // HTTP/1.1 외의 HTTP 요청은 적절한 응답 코드를 보내 거절합니다.
  if (!strcmp(startLine[2], "HTTP/1.1")) {
    printf("not HTTP/1.1: %s\n", startLine[2]);
    sprintf(response,
            "HTTP/1.1 505 HTTP Version Not Supported\r\n"
            "Content-Length: 0\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n");
    return 0;
  }

  /* Session Checking */
  char **cookieLine, **sessionID;
  int sessionChecked;

  int cookieIndex = FindLine(numOfRequestLines, splitedRequest, "Cookie:");
  if (cookieIndex != -1) {
    cookieLine = &splitedRequest[cookieIndex];
    SplitString(splitedRequest[cookieIndex], " ", &cookieLine);
    SplitString(cookieLine[2], "=", &sessionID);
  }

  sessionChecked = CheckSession(&g_Database, sessionID[1]);

  /* GET / */
  // 로그인된 경우, public/index.html 파일을 전송합니다.
  // 로그인되지 않은 경우, /login으로 리다이렉트합니다.
  if (!strcmp(startLine[0], "GET") && !strcmp(startLine[1], "/")) {
    if (sessionChecked == 0) {
      char* content;
      ReadTextFile("public/index.html", &content);

      sprintf(response,
              "HTTP/1.1 200 OK\r\n"
              "Content-Length: %d\r\n"
              "Content-Type: text/html\r\n"
              "\r\n%s",
              (int)strlen(content), content);
    } else {
      sprintf(response,
              "HTTP/1.1 302 Found\r\n"
              "Location: /login\r\n"
              "Content-Length: 0\r\n"
              "Content-Type: text/planin\r\n"
              "\r\n");
    }
  }

  /* GET /login */
  // 로그인된 경우, /로 리다이렉트합니다.
  // 로그인되지 않은 경우, public/login.html 파일을 전송합니다.
  if (!strcmp(startLine[0], "GET") && !strcmp(startLine[1], "/login")) {
    if (sessionChecked == 0) {
      sprintf(response,
              "HTTP/1.1 200 OK: Authorized\r\n"
              "Location: /\r\n"
              "Content-Length: 0\r\n"
              "Content-Type: text/plain\r\n"
              "\r\n");
    } else {
      char* content;
      ReadTextFile("public/login.html", &content);

      sprintf(response,
              "HTTP/1.1 302 Found\r\n"
              "Content-Length: %d\r\n"
              "Content-Type: text/html\r\n"
              "\r\n%s",
              (int)strlen(content), content);
    }
  }

  /* POST /login */
  // 로그인된 경우, 적절한 응답 코드를 보내 잘못된 접근임을 알립니다.
  // 로그인되지 않은 경우, 아이디와 비밀번호를 검증해 로그인합니다.
  // 쿠키를 이용해 세션 정보를 저장해야 합니다.
  // 성공한 경우, /로 리다이렉트합니다.
  if (!strcmp(startLine[0], "POST") && !strcmp(startLine[1], "/login")) {
    if (sessionChecked == 0) {
      sprintf(response,
              "HTTP/1.1 400 Bad Request\r\n"
              "Content-Length: 0\r\n"
              "Content-Type: text/plain\r\n"
              "\r\n");
    } else {
      char **splitedBody, **id, **pw, *newSessionID;
      SplitString(body, "&", &splitedBody);
      SplitString(splitedBody[0], "=", &id);
      SplitString(splitedBody[1], "=", &pw);
      int loginResult = Login(&g_Database, id[1], pw[1], &newSessionID);
      if (loginResult == 0) {
        sprintf(response,
                "HTTP/1.1 302 Found\r\n"
                "Set-Cookie: sessionID=%s\r\n"
                "Location: /\r\n"
                "Content-Length: 0\r\n"
                "Content-Type: text/plain\r\n"
                "\r\n",
                newSessionID);
      } else {
        sprintf(response,
                "HTTP/1.1 403 Forbidden\r\n"
                "Content-Length: 0\r\n"
                "Content-Type: text/plain\r\n"
                "\r\n");
      }
    }
  }

  /* GET /register */
  // 로그인된 경우, /로 리다이렉트합니다.
  // 로그인되지 않은 경우, public/register.html 파일을 전송합니다.
  if (!strcmp(startLine[0], "GET") && !strcmp(startLine[1], "/register")) {
    if (sessionChecked == 0) {
      sprintf(response,
              "HTTP/1.1 200 OK: Authorized\r\n"
              "Location: /\r\n"
              "Content-Length: 0\r\n"
              "Content-Type: text/plain\r\n"
              "\r\n");
    } else {
      char* content;
      ReadTextFile("public/register.html", &content);

      sprintf(response,
              "HTTP/1.1 302 Found\r\n"
              "Content-Length: %d\r\n"
              "Content-Type: text/html\r\n"
              "\r\n%s",
              (int)strlen(content), content);
    }
  }

  // Connection 헤더의 값에 따라, HTTP 응답을 보낸 후 TCP 연결을 끊거나 유지해야
  // 합니다. 간단함을 위해, keep-alive인 경우(대소문자 구분 X) 연결을 유지하고,
  // 그 외의 경우 연결을 즉시 끊는 것으로 합니다.
  char** connectionLine = NULL;
  int connectionIdx =
      FindLine(numOfRequestLines, splitedRequest, "Connection:");
  SplitString(splitedRequest[connectionIdx], " ", &connectionLine);
  if (!strncmp(connectionLine[1], "keep-alive", 10)) {
    return 0;
  } else {
    return 1;
  }

  /*
  char* dynamicContent = "Hello, world!";
  sprintf(response,
          "HTTP/1.1 200 OK\r\n"
          "Content-Length: %d\r\n"
          "Content-Type: text/plain\r\n"
          "\r\n"
          "%s",
          (int)strlen(dynamicContent), dynamicContent);
  */
}