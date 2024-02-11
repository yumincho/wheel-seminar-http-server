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
		if (sscanf(argv[1], "%d", &port) != 1 ||
			port < 0 || port > 65535) {

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

int DoSomething(HttpClient* client, const char* request, char* response) {
	/*
		아래에 있는 코드를 삭제하고, 여러분만의 코드를 작성해 주세요!

		request 매개변수에는 클라이언트가 보낸 HTTP 요청이 저장되어 있습니다.
		response 매개변수에는 여러분이 보낼 HTTP 응답을 저장하면 됩니다. 널 문자를 포함하여 최대 1024바이트까지만 저장할 수 있습니다.
		(더 긴 길이의 응답이 필요하다면 http.h 헤더 파일의 BUFFER_SIZE 매크로를 수정하면 됩니다.)

		이 함수의 리턴값의 의미는 다음과 같습니다:
		- 음수: 오류가 발생한 경우입니다. 클라이언트에 HTTP 응답을 보내지 않고 연결을 끊습니다.
		- 0: 클라이언트에 HTTP 응답을 보내고, 연결을 끊지 않고 대기합니다.
		- 양수: 클라이언트에 HTTP 응답을 보내고, 연결을 끊습니다.

		util.h 헤더 파일에 도움이 될 만한 함수를 몇 개 준비해 두었으니, 사용하시면 편리할겁니다!
		화이팅입니다.
	*/

	strcpy(response, "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello World"); // 예제 코드입니다.

	return 0;
}