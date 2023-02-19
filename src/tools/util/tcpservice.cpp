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
#include <log4pi.h>

using namespace std;
using namespace common::utility;

Logger tcpLogger{"tcpservice"};


#include "../include/tcpservice.h"

void startTCPService() {
    pthread_t threadId;
    int status = pthread_create(&threadId, NULL, tcpInterface, NULL);
    if (status != 0) {
        tcpLogger.error("tcpService::thread create failed %d--%s", status, strerror(errno)); fflush(stderr);
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
		tcpLogger.error("Could not create socket");
		return NULL;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(servicePort);

	int on = 1;
	if (setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		tcpLogger.error("could not set socket options on port %d", servicePort);
		return NULL;
	}
	//    if (setsockopt(socketDescriptor, IPPROTO_TCP, TCP_CORK,&on,sizeof(on))<0) {
	//        tcpLogger.error("could not set socket options on port %d",servicePort);
	//        return 9;
	//    }

	if (bind(socketDescriptor, (struct sockaddr *)&server, sizeof(server)) < 0) {
		tcpLogger.error("bind failed on port %d", servicePort);
		return NULL;
	}

	if (listen(socketDescriptor, SOMAXCONN) < 0) {
		tcpLogger.error("failed to listen on port %d", servicePort);  fflush(stdout);
		return NULL;
	}

	tcpLogger.info("listening on port %d", servicePort); fflush(stdout);
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
	tcpLogger.warn("broken pipe");
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

	tcpLogger.info("client connected"); 
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
		if (serviceDebug) tcpLogger.info("clinet disconnected");
	} else {
		tcpLogger.warn("client departed unexpectedly 200");
	}
	close(clientSocket);
}
