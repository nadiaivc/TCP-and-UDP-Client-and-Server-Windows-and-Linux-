#include <sys/types.h> 
#include <sys/socket.h> 
#include <netdb.h> 
#include <errno.h> 
#include <unistd.h>

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#pragma comment(lib, "ws2_32.lib")
#define MAX_SIZE 350000

typedef struct
{
	unsigned int number;
	unsigned char date1h;
	unsigned char date1m;
	unsigned char date1s;
	unsigned char date2[3];
	unsigned int messlen;
	unsigned int BBB;
	char* message;
} _msg;
_msg* msg; // array of desriptors every message
unsigned int count = 0;

int sock_err(const char* function, int s)
{
	int err;
	err = errno;
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}
unsigned int get_host_ipn(const char* name)
{
	struct addrinfo* addr = 0;
	unsigned int ip4addr = 0;
	if (0 == getaddrinfo(name, 0, 0, &addr))
	{
		struct addrinfo* cur = addr;
		while (cur)
		{
			if (cur->ai_family == AF_INET)
			{
				ip4addr = ((struct sockaddr_in*) cur->ai_addr)->sin_addr.s_addr;
				break;
			}
			cur = cur->ai_next;
		}
		freeaddrinfo(addr);
	}
	return ip4addr;
}

void send_msg_help(int i, int bytes, int sock, int what, int por)
{
	char nmsg[13] = { 0 }; unsigned int buf_msg;
	if (por == 1)
		buf_msg = htonl(what);
	else
		buf_msg = what;
	memcpy(nmsg, &buf_msg, bytes);
	send(sock, nmsg, bytes, 0);
}



char put[] = "put";
int send_msg(int sock)
{
	char ok[3] = { 0 };
	int success = 0, buf_msg = 0;
	send(sock, put, 3, 0);
	for (int i = 0; i < count; i++)
	{
		char nmsg[5] = { 0 }; unsigned int buf_msg;
		send_msg_help(i, 4, sock, msg[i].number, 1);
		send_msg_help(i, 1, sock, msg[i].date1h, 0);
		send_msg_help(i, 1, sock, msg[i].date1m, 0);
		send_msg_help(i, 1, sock, msg[i].date1s, 0);
	
		memcpy(nmsg, &msg[i].date2, 3);
		send(sock, nmsg, 3, 0);
	
		buf_msg = htonl(msg[i].BBB);
		memcpy(nmsg, &buf_msg, 4);
		send(sock, nmsg, 4, 0);

		msg[i].message[msg[i].messlen -1] = '\0';
		send(sock, msg[i].message, msg[i].messlen, 0); printf("mes: %s\n", msg[i].message);
		if (recv(sock, ok, 2, 0) == 2)
			success++;
		free(msg[i].message);
	}
	if (success == count)
		return 1;
	else
		return -1;
}




int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("Wrong\n");
		return 0;
	}
	char *ip = strtok(argv[1], ":");
	int port = atoi(strtok(NULL, " "));
	//char *ip = "127.0.0.1";
	//int port = 9000;
	struct sockaddr_in addr;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror("socket");
		exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = get_host_ipn(ip);
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		int i = 0;
		while ((connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) && i != 9)
		{
			printf("Waiting\n");
			sleep(1);
			i++;
		}
		if (i == 9)
		{
			perror("connect");
			close(sock);
			exit(2);
		}
	}
	
	//char c=fgetc(file);
	//	while (c!=EOF)
	//{
	//send(sock, number_msg, sizeof(int), 4);
	//number_msg++;
	char* string, *help;
	msg = (_msg*)malloc(sizeof(_msg));
	FILE* file = fopen(argv[2], "r");
	string = (char*)malloc(MAX_SIZE * sizeof(char));

	while (fgets(string, MAX_SIZE, file))
	{
		if (string[0] > ' ')
		{

			msg = (_msg*)realloc(msg, (count + 1) * sizeof(_msg));
			int len = strlen(string);printf("\n\n!!!!!!!!HERE   %d\n\n",len);
			string[len] = '\0';
			char* buf_msg = (char*)malloc(sizeof(char) * len);
			strncpy(buf_msg, string, len);
			buf_msg[len] = '\0';

			msg[count].number = count;

			msg[count].date1h = (unsigned int)(atoi(strtok(string, ":")));
			msg[count].date1m = (unsigned int)(atoi(strtok(NULL, ":")));
			msg[count].date1s = (unsigned int)(atoi(strtok(NULL, ":")));

			help = strtok(buf_msg, " ");


			msg[count].date2[0] = (unsigned int)(atoi(strtok(NULL, ":")));
			msg[count].date2[1] = (unsigned int)(atoi(strtok(NULL, ":")));
			msg[count].date2[2] = (unsigned int)(atoi(strtok(NULL, ":")));

			help = strtok(buf_msg + 16, " ");

			msg[count].BBB = (unsigned long)strtoul(strtok(NULL, " "), NULL, 0);

			strcpy(buf_msg, strtok(NULL, "\0"));
			msg[count].messlen = (unsigned int)(strlen(buf_msg));
			msg[count].message = (char*)malloc(msg[count].messlen * sizeof(char));
			strcpy(msg[count].message, buf_msg);
			
			count++;
			free(buf_msg);
		}
		
	}
	fclose(file);
	if (send_msg(sock) == 1)
		printf("OK, %d msg sent\n", count);
	else
		printf("NOT OK\n");
	close(sock);
	free(msg);
	free(string);
	return 0;
}
