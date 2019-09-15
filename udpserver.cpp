#include <sys/types.h> 
#include <sys/socket.h> 
#include <netdb.h> 
#include <errno.h> 
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
int codes[400] = { 0 };
typedef struct
{
	bool num_msg[120];
	unsigned int ip;
} _msg;
_msg msg[120]; // array of cli

int set_non_block_mode(int s)
{
	int fl = fcntl(s, F_GETFL, 0);
	return fcntl(s, F_SETFL, fl | O_NONBLOCK);
}

int sock_err(const char* function, int s)
{
	int err;
	err = errno;
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}

int  hash(char *S)
{
	int i, r = 0;
	while (*S)
	{
		r += (int)(*S);
		S++;
	}
	return r;
}

int main(int argc, char *argv[])
{
	
	if (argc != 3)
	{
		printf("Wrong\n");
		return 0;
	}

		int port_1 = atoi(argv[1]), port_2 = atoi(argv[2]);
		int port_arr[120];
		int  sock_arr[120];

		int num_str = 0;
		socklen_t addr_size;
		struct sockaddr_in addr;
		int end = 0;
	for (int i = 0; i <= port_2 - port_1; i++)
	{
		sock_arr[i] = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock_arr[i] < 0)
			return sock_err("socket", sock_arr[i]);
		
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		port_arr[i] = port_1 + i;
		addr.sin_port = htons(port_arr[i]);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(sock_arr[i], (struct sockaddr*) &addr, sizeof(addr)) < 0)
			return sock_err("bind", sock_arr[i]);
		set_non_block_mode(sock_arr[i]);
	}
	char info[50];
	fd_set rfd;
	fd_set wfd;
	int num_client=0;
	int nfds = sock_arr[0];
	
	struct timeval tv = { 10, 10 };
	do
	{
		char buffer[350024] = { 0 };
		int len = 0;
		socklen_t addrlen = sizeof(addr);
		FD_ZERO(&rfd);
		FD_ZERO(&wfd);
		for (int i = 0; i <= port_2 - port_1; i++)
		{
			FD_SET(sock_arr[i], &rfd);
			FD_SET(sock_arr[i], &wfd);
			if (nfds < sock_arr[i])
				nfds = sock_arr[i];
		}
		if (select(nfds + 1, &rfd, &wfd, 0, &tv) > 0)
		{
			for (int i = 0; i <= port_2 - port_1; i++)
			{
				if (FD_ISSET(sock_arr[i], &rfd))
				{
					int rcv = recvfrom(sock_arr[i], buffer, sizeof(buffer), 0, (struct sockaddr*) &addr, &addrlen);
					if (rcv > 0)
					{
						unsigned int ip = ntohl(addr.sin_addr.s_addr);
						//printf("OK\n");
						int this_cli = -1;
						strcat(info, "\0");
						for (int j = 0; j <= num_client; j++)
						{
							if (msg[j].ip == ip) {
								this_cli = j;//номер клиента, который отправил дейтаграмму 
								break;
							}
						}
						if (this_cli == -1)//новый клиент
						{
							this_cli = num_client;
							msg[num_client].ip = ip;
							memset(msg[num_client].num_msg, 0, 80 * sizeof(bool));
							num_client++;
						}
					
						FILE *file = fopen("msg.txt", "a");
						char nmsg[4] = { 0 };
						char printmsg[350560] = { 0 };
						char help[12] = { 0 };

						char number_msg[4] = { buffer[0], buffer[1], buffer[2], buffer[3] };
						unsigned int num_msg2;
						memcpy(&num_msg2, number_msg, 4);
						num_msg2 = ntohl(num_msg2);

							sprintf(info, "%u.%u.%u.%u: ", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
							char portprint[5] = { 0 };
							sprintf(portprint,"%d", port_arr[i]);
							strcat(info, portprint);strcat(info," ");
							strcat(printmsg, info);
							char check[350600];
							sprintf(check,"%d", num_msg2);
							unsigned char date;
							date = buffer[4]; int code; code = date;
							sprintf(help,"%d", code);
							 strcat(printmsg, help); strcat(printmsg, ":");

							date = buffer[5]; code = date;sprintf(help,"%d", code);
							strcat(printmsg, help); strcat(printmsg, ":");/////////!!!!!!!!!!!!!!

							date = buffer[6]; code = date;sprintf(help,"%d", code);
							strcat(printmsg, help); strcat(printmsg, " ");
							strcat(printmsg, " ");
							code = buffer[7];  sprintf(help,"%d", code); strcat(printmsg, help); strcat(printmsg, ":");
							code = buffer[8];  sprintf(help,"%d", code); strcat(printmsg, help); strcat(printmsg, ":");
							code = buffer[9];  sprintf(help,"%d", code); strcat(printmsg, help); strcat(printmsg, " ");

							unsigned int BBB; // parsing BBB
							char buf_msg3[4] = { buffer[10], buffer[11], buffer[12], buffer[13] };
							memcpy(&BBB, buf_msg3, 4);
							BBB = ntohl(BBB);
							sprintf(help, "%u", BBB);
							strcat(printmsg, help); strcat(printmsg, " ");

							char *message = (char*)malloc(350600 * sizeof(char)); // parsing message
							int i;
							for (i = 14; buffer[i] != '\0'; i++)
								message[i - 14] = buffer[i];
							message[i - 14] = '\0';
							strcat(printmsg, message);
							strcat(printmsg, "\n");

							strcat(check, printmsg);


							int cod = hash(check);
							for (i = 0; i <= num_str; i++) {
								if (codes[i] == cod)
									break;
							}
							if (i > num_str) {
								codes[i] = cod;
								num_str++;
								fputs(printmsg, file);
								fclose(file);
							}
						
							sendto(sock_arr[i], number_msg, 4, 0, (struct sockaddr*) &addr, addrlen);
							if (strcmp(message, "stop") == 0)
							{
								end = 1; break;
							}
							free(message);	
					}
				}
			}if (end == 1)break;
		}
	} while (1);
	for (int i = 0; i <= port_2 - port_1; i++)
		close(sock_arr[i]);
	
	
}


