#include <pthread.h>
#include <netinet/in.h>

#define BUFFER_SIZE 2048
#define TIMEOUT 5000 // 5초

typedef struct {
	int Socket;
	struct sockaddr_in Address;
	char AddressString[INET_ADDRSTRLEN];
	pthread_t Thread;
} HttpClient;

typedef struct HttpClientNode {
	HttpClient Client;
	struct HttpClientNode* Previous;
	struct HttpClientNode* Next;
	pthread_mutex_t* ClientsMutex;
	int (*Callback)(HttpClient*, const char*, char*);
} HttpClientNode;

typedef struct {
	int Socket;
	struct sockaddr_in Address;
	HttpClientNode* Clients;
	pthread_mutex_t ClientsMutex;
} HttpServer;

int OpenHttpServer(HttpServer* server, int port, int backlog);
int AcceptHttpClient(HttpServer* server, HttpClient** client, int callback(HttpClient*, const char*, char*));