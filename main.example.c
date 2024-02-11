/**
 * main.c의 예시 코드입니다.
 * 어떻게 할 지 감이 잘 안 잡히시거나, 이미 코드를 완성하신 경우에만 읽어보시는 것을 추천합니다.
 */

#include "http.h"
#include "util.h"

// 추가적으로 필요한 헤더 파일이 있다면 여기에 추가해 주세요.
#include <ctype.h>
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
	printf("클라이언트 %s가 요청한 내용:\n%s\n", client->AddressString, request);

	// Headers와 Body를 나눕니다.
	char** headerAndBody;
	int headerAndBodyCount = SplitString(request, "\r\n\r\n", &headerAndBody);

	// 아까 나눈 Headers를 다시 줄 단위로 나눕니다.
	char** headers;
	int headersCount = SplitString(headerAndBody[0], "\r\n", &headers);

	// Headers의 첫 번째 줄은 사실 Start Line입니다. 공백을 기준으로 나눕니다.
	char** startLine;
	SplitString(headers[0], " ", &startLine);

	// HTTP/1.1이 아니면 505 HTTP Version Not Supported를 보냅니다.
	if (strcmp(startLine[2], "HTTP/1.1") != 0) {
		strcpy(response, "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 0\r\n\r\n");
		return 1;
	}

	// 여기에 각각 Connection, Cookie 헤더의 값을 저장할 것입니다.
	char* connectionHeader = NULL;
	char* cookieHeader = NULL;

	// 나눈 Headers를 순회하며 Connection, Cookie 헤더의 값을 찾습니다.
	for (int i = 1; i < headersCount; ++i) {
		char** nameAndValue;
		int nameAndValueCount = SplitString(headers[i], ": ", &nameAndValue);

		// Header의 이름은 대소문자 구분을 하지 않습니다. 편의를 위해 소문자로 일괄 변환하여 비교합니다.
		for (int j = 0; j < strlen(nameAndValue[0]); ++j) {
			nameAndValue[0][j] = tolower(nameAndValue[0][j]);
		}

		if (strcmp(nameAndValue[0], "connection") == 0) {
			// Connection 헤더의 값을 소문자로 변환하여 저장합니다.
			connectionHeader = nameAndValue[1];
			for (int j = 0; j < strlen(connectionHeader); ++j) {
				connectionHeader[j] = tolower(connectionHeader[j]);
			}
		} else if (strcmp(nameAndValue[0], "cookie") == 0) {
			// Cookie 헤더의 값을 저장합니다.
			cookieHeader = nameAndValue[1];
		}
	}

	// Cookie 헤더의 값을 ; 단위로 나눕니다. 브라우저가 보내는 쿠키는 ; 단위로 구분됩니다.
	char** cookies = NULL;
	int cookiesCount = 0;
	if (cookieHeader != NULL) {
		cookiesCount = SplitString(cookieHeader, "; ", &cookies);
	}

	// 쿠키 중에서 sessionid라는 이름을 가진 쿠키를 찾아, 그 값을 sessionIDCookie에 저장합니다.
	// 즉, sessionIDCookie에는 세션 아이디가 저장됩니다.
	char* sessionIDCookie = NULL;
	if (cookies) {
		for (int i = 0; i < cookiesCount; ++i) {
			// 이름과 값이 =로 구분되어 있으므로, = 단위로 나눕니다.
			char** nameAndValue;
			int nameAndValueCount = SplitString(cookies[i], "=", &nameAndValue);

			if (strcmp(nameAndValue[0], "sessionid") == 0) {
				sessionIDCookie = nameAndValue[1];
				break;
			}
		}
	
	}

	// 세션이 유효한지 확인합니다. 세션이 유효하지 않다면 isLoggedIn에 0을, 유효하다면 1을 저장합니다.
	// C언어에서 0은 falsy이고, 0이 아닌 값은 truthy이므로, if문을 사용해 간편하게 로그인 여부를 확인할 수 있습니다.
	int isLoggedIn = !(sessionIDCookie ? CheckSession(&g_Database, sessionIDCookie) : -1);

	// 여기부터 HTTP 요청을 본격적으로 파싱하여 적절한 응답을 클라이언트로 보내게 됩니다.
	if (strcmp(startLine[0], "GET") == 0 && strcmp(startLine[1], "/") == 0) {
		if (isLoggedIn) {
			char* indexHTML;
			int indexHTMLLength = ReadTextFile("public/index.html", &indexHTML);

			strcpy(response, "HTTP/1.1 200 OK\r\nContent-Length: ");
			sprintf(response + strlen(response), "%d", indexHTMLLength);
			strcat(response, "\r\n\r\n");
			strcat(response, indexHTML);
		} else {
			strcpy(response, "HTTP/1.1 302 Found\r\nLocation: /login\r\nContent-Length: 0\r\n\r\n");
		}
	} else if (strcmp(startLine[0], "GET") == 0 && strcmp(startLine[1], "/login") == 0) {
		if (isLoggedIn) {
			strcpy(response, "HTTP/1.1 302 Found\r\nLocation: /\r\nContent-Length: 0\r\n\r\n");
		} else {
			char* loginHTML;
			int loginHTMLLength = ReadTextFile("public/login.html", &loginHTML);

			strcpy(response, "HTTP/1.1 200 OK\r\nContent-Length: ");
			sprintf(response + strlen(response), "%d", loginHTMLLength);
			strcat(response, "\r\n\r\n");
			strcat(response, loginHTML);
		}
	} else if (strcmp(startLine[0], "POST") == 0 && strcmp(startLine[1], "/login") == 0) {
		if (isLoggedIn) {
			strcpy(response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
		} else {
			// login.html에서 로그인 버튼을 눌렀다면, Body의 형식은 다음과 같습니다: id=아이디&password=비밀번호
			// 따라서, 먼저 &로 나눈 뒤 =로 나누면 아이디와 비밀번호만 얻을 수 있습니다.
			char** body;
			int bodyCount = SplitString(headerAndBody[1], "&", &body);

			// 여기에 아이디와 비밀번호가 저장됩니다.
			char* id = NULL;
			char* password = NULL;

			for (int i = 0; i < bodyCount; ++i) {
				char** nameAndValue;
				int nameAndValueCount = SplitString(body[i], "=", &nameAndValue);

				if (strcmp(nameAndValue[0], "id") == 0) {
					id = nameAndValue[1];
				} else if (strcmp(nameAndValue[0], "password") == 0) {
					password = nameAndValue[1];
				}
			}

			// 아이디와 비밀번호가 모두 입력되었다면, 로그인을 시도합니다.
			if (id != NULL && password != NULL) {
				// 로그인에 성공한 경우 여기에 세션 아이디가 저장됩니다.
				char* sessionID;
				if (Login(&g_Database, id, password, &sessionID) == 0) {
					strcpy(response, "HTTP/1.1 302 Found\r\nLocation: /\r\nSet-Cookie: sessionid=");
					strcat(response, sessionID);
					strcat(response, "\r\nContent-Length: 0\r\n\r\n");
				} else {
					strcpy(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
				}
			} else {
				strcpy(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
			}
		}
	} else if (strcmp(startLine[0], "GET") == 0 && strcmp(startLine[1], "/register") == 0) {
		if (isLoggedIn) {
			strcpy(response, "HTTP/1.1 302 Found\r\nLocation: /\r\nContent-Length: 0\r\n\r\n");
		} else {
			char* registerHTML;
			int registerHTMLLength = ReadTextFile("public/register.html", &registerHTML);

			strcpy(response, "HTTP/1.1 200 OK\r\nContent-Length: ");
			sprintf(response + strlen(response), "%d", registerHTMLLength);
			strcat(response, "\r\n\r\n");
			strcat(response, registerHTML);
		}
	} else if (strcmp(startLine[0], "POST") == 0 && strcmp(startLine[1], "/register") == 0) {
		if (isLoggedIn) {
			strcpy(response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
		} else {
			char** body;
			int bodyCount = SplitString(headerAndBody[1], "&", &body);

			char* id = NULL;
			char* password = NULL;

			for (int i = 0; i < bodyCount; ++i) {
				char** nameAndValue;
				int nameAndValueCount = SplitString(body[i], "=", &nameAndValue);

				if (strcmp(nameAndValue[0], "id") == 0) {
					id = nameAndValue[1];
				} else if (strcmp(nameAndValue[0], "password") == 0) {
					password = nameAndValue[1];
				}
			}

			// 아이디와 비밀번호가 모두 입력되었다면, 회원가입을 시도합니다.
			if (id != NULL && password != NULL) {
				if (Register(&g_Database, id, password) == 0) {
					strcpy(response, "HTTP/1.1 302 Found\r\nLocation: /login\r\nContent-Length: 0\r\n\r\n");
				} else {
					strcpy(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
				}
			} else {
				strcpy(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
			}
		}
	} else if (strcmp(startLine[0], "GET") == 0 && strcmp(startLine[1], "/text") == 0) {
		if (isLoggedIn) {
			// 사용자가 저장한 텍스트를 데이터베이스에서 가져옵니다.
			// GetUserText는 세션 아이디를 받아, 해당 세션 아이디에 해당하는 사용자가 저장한 텍스트를 반환합니다.
			const char* result = GetUserText(&g_Database, sessionIDCookie);

			strcpy(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ");
			sprintf(response + strlen(response), "%d", (int)strlen(result));
			strcat(response, "\r\n\r\n");
			strcat(response, result);
		} else {
			strcpy(response, "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\n\r\n");
		}
	} else if (strcmp(startLine[0], "POST") == 0 && strcmp(startLine[1], "/text") == 0) {
		if (isLoggedIn) {
			// 사용자가 보낸 텍스트를 데이터베이스에 저장합니다.
			// SetUserText는 GetUserText와 마찬가지로 세션 아이디를 받습니다.
			SetUserText(&g_Database, sessionIDCookie, headerAndBody[1]);

			strcpy(response, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
		} else {
			strcpy(response, "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\n\r\n");
		}
	} else {
		// 메서드가 잘못 됐거나, 리소스 경로가 잘못된 경우 404 Not Found를 보냅니다.
		char* errorHTML;
		int errorHTMLLength = ReadTextFile("public/404.html", &errorHTML);

		strcpy(response, "HTTP/1.1 404 Not Found\r\nContent-Length: ");
		sprintf(response + strlen(response), "%d", errorHTMLLength);
		strcat(response, "\r\n\r\n");
		strcat(response, errorHTML);
	}

	if (connectionHeader != NULL) {
	 	if (strcmp(connectionHeader, "keep-alive") == 0) {
			// Connection 헤더의 값이 keep-alive라면, 클라이언트와의 연결을 유지합니다.
			// http.h 파일에 정의된 TIMEOUT 값만큼 연결을 유지하다가, 클라이언트로부터 요청이 오면 다시 응답을 보냅니다.
			// 마지막으로 요청이 오고 TIMEOUT 값만큼의 시간이 지나면 연결이 해제됩니다.
			return 0;
		} else {
			// Connection 헤더의 값이 keep-alive가 아니라면, 연결을 끊습니다.
			return 1;
		}
	} else {
		// HTTP/1.1에서는 Connection 헤더의 값이 설정되지 않으면 기본적으로 keep-alive입니다.
		return 0;
	}
}