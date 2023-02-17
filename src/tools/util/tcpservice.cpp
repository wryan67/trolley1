#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

//#include <sys/mman.h>

//c++ try/catch
#include <stdexcept>
#include <iostream>
#include <thread>

using namespace std;

#include "../include/tcpservice.h"

void startTCPService() {
    pthread_t threadId;
    int status = pthread_create(&threadId, NULL, tcpInterface, NULL);
    if (status != 0) {
        fprintf(stderr, "tcpService::thread create failed %d--%s\n", status, strerror(errno)); fflush(stderr);
        exit(9);
    }
    pthread_detach(threadId);
}

void *tcpInterface(void *) {
	int socketDescriptor, newSocket, socketSize, client;
	int *newSock;
	struct sockaddr_in server;

	signal(SIGPIPE, signal_SIGPIPE_handler);


	usleep(1000);

	socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (socketDescriptor < 0) {
		printf("Could not create socket\n");
		return NULL;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(servicePort);

	int on = 1;
	if (setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		printf("could not set socket options on port %d\n", servicePort);
		return NULL;
	}
	//    if (setsockopt(socketDescriptor, IPPROTO_TCP, TCP_CORK,&on,sizeof(on))<0) {
	//        printf("could not set socket options on port %d\n",servicePort);
	//        return 9;
	//    }

	if (bind(socketDescriptor, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("bind failed on port %d\n", servicePort);
		return NULL;
	}

	if (listen(socketDescriptor, SOMAXCONN) < 0) {
		printf("failed to listen on port %d\n", servicePort);  fflush(stdout);
		return NULL;
	}

	printf("listening on port %d\n", servicePort); fflush(stdout);
	socketSize = sizeof(struct sockaddr_in);

	while (newSocket = accept(socketDescriptor, (struct sockaddr *)&client, (socklen_t*)&socketSize)) {
		if (newSocket < 0) {
			perror("accept failed"); fflush(stderr);
		}

		// pthread_t clientThread;

		// newSock = (int*)malloc(sizeof(int*));
		// *newSock = newSocket;

		thread(serviceChildMethod,newSocket).detach();
		// if (pthread_create(&clientThread, NULL, serviceChildMethod, (void*)newSock) < 0) {
		// 	perror("could not create client thread"); fflush(stderr);
		// 	return NULL;
		// } else {
		// 	// add clientThread to active clinet list
		// }
	}
	return NULL;
}


void signal_SIGPIPE_handler(int signum) {
	printf("broken pipe\n"); fflush(stdout);
}

void prompt(int clientSocket) {
	const char *prompt = "CMD> ";

	write(clientSocket, prompt, strlen(prompt));
	fsync(clientSocket);
}

void serviceChildMethod(int clientSocket)
{
	char tmpstr[2048];
	long xwritten;

	int recvSize;

	fprintf(stderr,"clinet connected\n"); fflush(stdout);
	prompt(clientSocket);

	while ((recvSize = recv(clientSocket, tmpstr, sizeof(tmpstr), 0)) > 0) {
		tmpstr[strcspn(tmpstr, "\n")] = 0;
		tmpstr[strcspn(tmpstr, "\r")] = 0;

		const char *rs=executeServiceCommand(tmpstr);

		if (rs == NULL) {
			recvSize == 0;
			break;
		}

		xwritten = write(clientSocket, rs, strlen(rs));
		write(clientSocket, "\n", 1);

		memset(tmpstr, 0, sizeof(tmpstr));
		prompt(clientSocket);
	}

	if (recvSize >= 0) {
		if (serviceDebug) fprintf(stderr,"clinet disconnected\n"); fflush(stdout);
	} else {
		fprintf(stderr,"client departed unexpectedly 200\n"); fflush(stdout);
	}
	close(clientSocket);
}
