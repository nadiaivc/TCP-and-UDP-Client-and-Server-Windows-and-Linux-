#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include <winsock2.h> 
#include <iostream>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")
char info[20];
int numEvents = 0;
int sockets[200];
int listener;

int init()
{
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
}


int recv_string(int cs, int here)
{
	FILE *file = fopen("msg.txt", "a");
	const char ok[3] = "ok";
	char buffer[4] = { 0 };
	char msg[350080] = { 0 };
	char help[12] = { 0 };
	int ret; unsigned int date; // parsing date
	unsigned char date2;
	char put[4];
	if (here == 0) {
		ret = recv(cs, put, 3, 0);
		put[ret] = '\0';

		if (strcmp(put, "put") == 0)
			printf("get PUT\n");
		else 
			return 2;
	}
	strcat(msg, info);

	int len_msg;
	unsigned int num_msg; 
	
	if(recv(cs, buffer, 4, 0)==0)
		return 2;
	memcpy(&num_msg, buffer, 4); //numder 4 bytes
	num_msg = ntohl(num_msg);
	
	len_msg = recv(cs, buffer, 1, 0); //date 1 byte
	if (len_msg < 0)
		return 2;
	memcpy(&date2, buffer, 1); int code; code = date2; 
	 _itoa(code, help, 10); strcat(msg, help); strcat(msg, ":");

	len_msg = recv(cs, buffer, 1, 0); //date 1 byte
	memcpy(&date2, buffer, 1); code = date2;
	 _itoa(code, help, 10); strcat(msg, help); strcat(msg, ":");

	len_msg = recv(cs, buffer, 1, 0); //date 1 byte
	memcpy(&date2, buffer, 1);
	code = date2;  _itoa(code, help, 10); strcat(msg, help); strcat(msg, " ");

	len_msg = recv(cs, buffer, 3, 0); //date 3 bytes
	code = buffer[0];  _itoa(code, help, 10); strcat(msg, help); strcat(msg, ":");
	code = buffer[1];  _itoa(code, help, 10); strcat(msg, help); strcat(msg, ":");
	code = buffer[2];  _itoa(code, help, 10); strcat(msg, help); strcat(msg, " ");

	unsigned int BBB; //BBB 4 bytes
	len_msg = recv(cs, buffer, 4, 0);
	memcpy(&BBB, buffer, 4);
	BBB = ntohl(BBB);
	sprintf(help, "%u", BBB); 
	strcat(msg, help); strcat(msg, " ");

	char *message = (char*)malloc(350000 * sizeof(char)); //message
	len_msg = recv(cs, message, 350000, 0); message[len_msg] = '\0';
	printf("%d\n", len_msg); strcat(msg, message);
	strcat(msg, "\n"); 
	
	
	fputs(msg, file);
	send(cs, "ok", 2, 0);

	printf("%s\n",msg);
	fclose(file);
	

	if (strcmp(message, "stop")==0) {
		for (int i = 0; i < numEvents; i++)
			closesocket(sockets[i]);
		closesocket(listener);
		printf("Client disconnected\n");
		WSACleanup(); free(message);
		return 0;
	}free(message);
	return 1;
}

int set_non_block_mode(int s)
{
	unsigned long mode = 1;
	return ioctlsocket(s, FIONBIO, &mode);
}
int sock_err(const char* function, int s)
{
	int err; 
	err = WSAGetLastError(); 
		
		fprintf(stderr, "%s: socket error: %d\n", function, err); 
		return -1;
}


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Wrong\n");
		return 0;
	}
	int port = atoi(argv[1]);
	init();
	struct sockaddr_in addr; int ret;
	listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listener < 0)
		return sock_err("socket", listener);
	set_non_block_mode(listener);
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listener, (struct sockaddr*) &addr, sizeof(addr)) < 0)
		return sock_err("bind", listener);
	if (listen(listener, 10) < 0)
		return sock_err("listen", listener);
	
	
	WSAEVENT events[200];
	WSAEVENT newEvent = WSACreateEvent();
	WSAEventSelect(listener, newEvent, FD_ACCEPT);
	sockets[numEvents] = listener;
	events[numEvents] = newEvent;
	numEvents++;
	int addrlen = sizeof(addr);
	
	int fail = 1;
	while (fail!=0) {
		
		int index = 0;
		for (int i = index; i < numEvents; i++) {
			ret = WSAWaitForMultipleEvents(1, &events[i],FALSE, 0, FALSE);
			if (ret == WSA_WAIT_FAILED)
				continue;

			WSANETWORKEVENTS networkEvents;
			WSAEnumNetworkEvents(sockets[i], events[i], &networkEvents);
			WSAResetEvent(events[i]);

			if (networkEvents.lNetworkEvents & FD_ACCEPT) {
				if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
					printf("FD_ACCEPT failed\n");
					continue;
				}
				if (numEvents == WSA_MAXIMUM_WAIT_EVENTS) {
					printf("Too many connection\n");
					continue;
				}

				int client = accept(sockets[i], (struct sockaddr*)&addr, &addrlen);
				
				newEvent = WSACreateEvent();
				WSAEventSelect(client, newEvent, FD_READ | FD_CLOSE);

				events[numEvents] = newEvent;
				sockets[numEvents] = client;
				numEvents++;
				printf("New client connected %d\n", numEvents-1);
			}

			if (networkEvents.lNetworkEvents & FD_READ) {
				if (networkEvents.iErrorCode[FD_READ_BIT] != 0)
				{
					printf("FD_READ failed\n");
					continue;
				}
					int ip = ntohl(addr.sin_addr.s_addr); 
					sprintf(info, "%u.%u.%u.%u: ", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
					char portStr[5] = { 0 };
					_itoa(port, portStr, 10);
					strcat(info, portStr);
					strcat(info, "\0");
					fail = 1;
					int here = -1;
					do
					{
						here++;
						fail = recv_string(sockets[i], here);
					} while (fail == 1);
			}

			if (networkEvents.lNetworkEvents & FD_CLOSE) {
				closesocket(sockets[i]);
				printf("Client disconnected\n");
			}
		}
	}
	return 0;
}

