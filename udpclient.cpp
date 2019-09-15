
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include <winsock2.h> 
// Директива линковщику: использовать библиотеку сокетов 
#pragma comment(lib, "ws2_32.lib") 

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#define MAX_SIZE 350000

char message[] = "put";
unsigned int suc = 0;

typedef struct
{
	unsigned int number;
	unsigned char date1h;
	unsigned char date1m;
	unsigned char date1s;
	unsigned char date2[3];
	unsigned long BBB;
	unsigned int mess_buf;
	char* message;
} _msg;
_msg* msg; // array of desriptors every message
unsigned int count = 0;
bool *succes;
int init()
{
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));

}
int sock_err(const char* function, int s)
{
	int err;
	err = WSAGetLastError();
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}
void deinit()
{
	WSACleanup();
}

void send_msg_help(int bytes, int sock, int what, char datagram[], int pos, int por)
{
	char nmsg[15] = { 0 }; int buf_msg;
	if (por == 0)
		buf_msg = what;
	else
		buf_msg = htonl(what);
	memcpy(nmsg, &buf_msg, bytes);
	for (int j = pos; j < pos + bytes; j++)
		datagram[j] = nmsg[j - pos];
}


void send_msg(int sock, struct sockaddr_in *addr, int i)
{
	int len_datagram = 24 + msg[i].mess_buf;
	char *datagram = (char *)calloc(len_datagram, sizeof(char)); // 14 bytes = 4 - number,3+3 =6 - date, 4 uint, N for message len
	int pos = 0;
	char nmsg[5] = { 0 };

	int j;
	send_msg_help(4, sock, msg[i].number, datagram, pos, 1); pos += 4;
	send_msg_help(1, sock, msg[i].date1h, datagram, pos, 0); pos++;
	send_msg_help(1, sock, msg[i].date1m, datagram, pos, 0); pos++;
	send_msg_help(1, sock, msg[i].date1s, datagram, pos, 0); pos++;

	memcpy(nmsg, &msg[i].date2, 3);
	for (int j = pos; j < pos + 3; j++)
		datagram[j] = nmsg[j - pos];
	pos += 3;

	send_msg_help(4, sock, msg[i].BBB, datagram, pos, 1); pos += 4;

	for (j = 0; j < msg[i].mess_buf; j++)
		datagram[j + 14] = msg[i].message[j];

	datagram[j + 14] = '\0';
	int res = sendto(sock, datagram, len_datagram + 1, 0, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
	if (!res)
	{
		printf("FAIL TO SEND. INDEX = %u", i);
		sock_err("sendto", sock);
	}
	free(datagram); 
}

int recv_response(int s)
{
	char datagram[10];
	struct timeval tv = { 0, 100 * 1000 }; // 100 ms delay ;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(s, &fds);

	int res = select(s + 1, &fds, 0, 0, NULL);
	if (res > 0) // Данные есть, считывание их
	{
		struct sockaddr_in addr;
		int addrlen = sizeof(addr);
		int received = recvfrom(s, datagram, sizeof(datagram), 0, (struct sockaddr *)&addr, &addrlen);
		if (received <= 0) // Ошибка считывания полученной дейтаграммы
		{
			sock_err("recvfrom", s);
			return 0;
		}
		else
		{

			int index = 0;
			memcpy(&index, datagram, 4);
			index = ntohl(index);
			succes[index] = true;

			int count_msg = 0; // Calculate successfull messages
			for (int i = 0; i < count; i++)
				if (succes[i] == true)
					count_msg++;
			if (count_msg >= 20 || count_msg == count)
				suc = 21;
			else
				suc++;
		}
		return 0;
	}
	else if (res == 0) // Данных в сокете нет, возврат ошибки
		return 0;
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
	init();
	struct sockaddr_in addr;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("socket");
		exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	send(sock, message, sizeof(message), 0);
	////////////////////////////////////////
	char *help;
	msg = (_msg*)malloc(sizeof(_msg));
	FILE* file = fopen(argv[2], "r"); 
	char string[MAX_SIZE] = { 0 };
	char* buf_msg = 0; char c = fgetc(file); int i = 0;
	while (1) {
		while (c != '\n'&& c != EOF)
		{
			string[i] = c;
			i++;
			c = fgetc(file);

		}
		string[i] = '\0';
		i = 0;
		if (c == EOF)
			break;
		c = fgetc(file);
		if (string[0] != '\0')
		
		{
			msg = (_msg*)realloc(msg, (count + 1) * sizeof(_msg));
			int clen = strlen(string);
			string[clen] = '\0';
			buf_msg = (char*)malloc(sizeof(char) * clen);
			strcpy(buf_msg, string);
			buf_msg[clen] = '\0';

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
			clen = strlen(buf_msg);
			msg[count].mess_buf = clen;
			msg[count].message = (char*)malloc(clen * sizeof(char));
			strcpy(msg[count].message, buf_msg);
			
			count++;
		}string[0] = '\0';
	}
	fclose(file);
	///////////////////////////////////////////////////////////////////////////////////
	succes = (bool*)calloc(count, sizeof(bool));
	while (1)
	{
		for (int i = 0; i < count; i++)
			send_msg(sock, &addr, i);

		recv_response(sock);
		if (suc >= 20 || count == suc)
		{
			printf("OK. ALL SENT\n");
			closesocket(sock);
			free(msg);
			free(succes);
			return 0;
		}
	}
	for (int i = 0; i < count; i++)
		free(msg[i].message);
	deinit(); free(buf_msg);
	system("pause");
}
