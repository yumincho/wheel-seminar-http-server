#include "http.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

static void* RespondHttpRequest(void* arg);

int OpenHttpServer(HttpServer* server, int port, int backlog) {
	memset(server, 0, sizeof(HttpServer));

	server->Clients = malloc(sizeof(HttpClientNode)); // Dummy node
	if (server->Clients == NULL) {
		return -1;
	} else {
		memset(server->Clients, 0, sizeof(HttpClientNode));
	}

	server->Address = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};
	if ((server->Socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	if (bind(server->Socket, (struct sockaddr*)&server->Address, sizeof(server->Address)) < 0) {
		return -1;
	}
	if (listen(server->Socket, backlog) < 0) {
		return -1;
	}

	pthread_mutex_init(&server->ClientsMutex, NULL);

	return 0;
}
int AcceptHttpClient(HttpServer* server, HttpClient** client, int (*callback)(HttpClient*, const char*, char*)){
	HttpClientNode* clientNode = (HttpClientNode*)malloc(sizeof(HttpClientNode));
	if (clientNode == NULL) {
		return -1;
	} else {
		memset(clientNode, 0, sizeof(HttpClientNode));
		clientNode->ClientsMutex = &server->ClientsMutex;
		clientNode->Callback = callback;
	}

	socklen_t addressLength = sizeof(clientNode->Client.Address);
	if ((clientNode->Client.Socket = accept(server->Socket, (struct sockaddr*)&clientNode->Client.Address, &addressLength)) < 0) {
		free(clientNode);
		return -1;
	} else {
		inet_ntop(AF_INET, &clientNode->Client.Address.sin_addr, clientNode->Client.AddressString, INET_ADDRSTRLEN);
	}

	pthread_mutex_lock(&server->ClientsMutex);

	HttpClientNode* lastClient = server->Clients;
	while (lastClient->Next != NULL) {
		lastClient = lastClient->Next;
	}
	lastClient->Next = clientNode;
	clientNode->Previous = lastClient;

	int clientCount = 0;
	HttpClientNode* currentClient = server->Clients;
	while (currentClient->Next != NULL) {
		clientCount++;
		currentClient = currentClient->Next;
	}

	printf("클라이언트 %s와 연결되었습니다. 연결된 클라이언트 수: %d개\n", clientNode->Client.AddressString, clientCount);

	pthread_mutex_unlock(&server->ClientsMutex);

	pthread_create(&clientNode->Client.Thread, NULL, &RespondHttpRequest, clientNode);

	*client = &clientNode->Client;
	return 0;
}

void* RespondHttpRequest(void* arg) {
	HttpClientNode* const clientNode = (HttpClientNode*)arg;

	const struct timeval timeout = { TIMEOUT / 1000, TIMEOUT % 1000 };
	setsockopt(clientNode->Client.Socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	while (1) {
		printf("클라이언트 %s의 HTTP 요청을 기다립니다.\n", clientNode->Client.AddressString);

		char request[BUFFER_SIZE] = { 0, };
		const ssize_t recvResult = recv(clientNode->Client.Socket, request, BUFFER_SIZE, 0);
		if (recvResult < 0) {
			printf("클라이언트 %s의 HTTP 요청을 받지 못했습니다.\n", clientNode->Client.AddressString);
			break;
		} else if (recvResult == 0) {
			printf("클라이언트 %s가 연결을 끊었습니다.\n", clientNode->Client.AddressString);
			break;
		}

		char response[BUFFER_SIZE] = { 0, };
		const int callbackResult = clientNode->Callback(&clientNode->Client, request, response); // <0: Error, 0: Continue, >0: Finish
		if (callbackResult < 0) {
			break;
		}

		if (send(clientNode->Client.Socket, response, strlen(response), 0) < 0) {
			printf("클라이언트 %s에 HTTP 응답을 보내지 못했습니다.\n", clientNode->Client.AddressString);
			break;
		}
		if (callbackResult > 0) {
			break;
		}
	}

	pthread_mutex_lock(clientNode->ClientsMutex);

	clientNode->Previous->Next = clientNode->Next;
	if (clientNode->Next != NULL) {
		clientNode->Next->Previous = clientNode->Previous;
	}

	int clientCount = 0;
	HttpClientNode* currentClient = clientNode;
	while (currentClient->Previous != NULL) {
		currentClient = currentClient->Previous;
	}
	while (currentClient->Next != NULL) {
		clientCount++;
		currentClient = currentClient->Next;
	}

	printf("클라이언트 %s와 연결이 해제되었습니다. 연결된 클라이언트 수: %d개\n", clientNode->Client.AddressString, clientCount);

	pthread_mutex_unlock(clientNode->ClientsMutex);

	close(clientNode->Client.Socket);
	free(clientNode);
	return NULL;
}