#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <syslog.h>

#include "fairplay.h"

#define MAX_LISTEN_NUM 5
#define SEND_BUF_SIZE 1024
#define RECV_BUF_SIZE 1024
#define LISTEN_PORT 19999

//#define RUN_DAEMON 

#ifdef DEBUG
#ifdef RUN_DAEMON
#define dprintf(fmt, args...) syslog(LOG_DEBUG, fmt, ##args)
#else
#define dprintf printf
#endif

#else
#define dprintf(...)
#endif

#ifdef RUN_DAEMON
#define myprintf(fmt, args...) syslog(LOG_INFO, fmt, ##args)
#else
#define myprintf(fmt, args...) fprintf(stderr, fmt, ##args)
#endif

static void print_buf(unsigned char *buf, int len)
{
#ifdef DEBUG
	dprintf("buflen:%d\n", len);
	int i;
	for (i = 0; i < len; i++) {
		dprintf("0x%02x ", buf[i]);
	}
	dprintf("\n");
#endif
}

static int correct_len[] = { 18, 166, 74 };
void *handle_one_client(void *arg)
{
	int ret;
	unsigned char sendbuf[SEND_BUF_SIZE] = { 0 };
	unsigned char recvbuf[RECV_BUF_SIZE] = { 0 };
	int sendlen = 0;
	int recvlen = 0;
	int cmd;
	unsigned char c;
	unsigned char *outbuf = NULL;
	int n = 0;
	int app_sock = *(int*)arg;

	unsigned char hwinfo[24];
	unsigned char *sapbuf = NULL;

	struct timeval tv;

	tv.tv_sec = 10;  /* 10 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors

	setsockopt(app_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

	do {
		n++;
		ret = 0;

		//receive data
		recvlen = RECV_BUF_SIZE - 1;
		// in theory there are chances that the packet is fragmented by routers,
		// but since the packets are small and if it happens client can just retry
		// we don't bother to write a more robust receive func
		recvlen = recv(app_sock, recvbuf, recvlen, 0);

		if (recvlen <= 0) {
			dprintf("recv error, errno=%d\n", errno);
			ret = 0 - n;
		} else {
			unsigned int len;
			cmd = recvbuf[0];
			len = recvbuf[1];
			dprintf("recv cmd=%d, len = %d, recvlen = %d\n", cmd, len, recvlen);

			if (cmd != n || recvlen != len || len != correct_len[n-1]) {
				dprintf("Invalid input, cmd=%d, len = %d, recvlen = %d\n",
					 cmd, len, recvlen);
				ret = 0 - n;
			}
			print_buf(recvbuf, recvlen);
		}
		if (ret != 0) break;

		switch (n) {
			case 1:
				airplay_load_hwinfo(hwinfo);
				airplay_initialize_sap(hwinfo, &sapbuf);
				airplay_process_fpsetup_message(3, hwinfo, sapbuf, 
						recvbuf + 2, &outbuf, &sendlen, &c);
				break;
			case 2:
				airplay_process_fpsetup_message(3, hwinfo, sapbuf, 
						recvbuf + 2, &outbuf, &sendlen, &c);
				break;
			case 3:
				airplay_get_encryption_key(sapbuf, recvbuf + 2, recvlen - 2, 
						&outbuf, &sendlen);
				break;
		}

		if (outbuf) {
			memcpy(sendbuf, outbuf, sendlen);
			print_buf(sendbuf, sendlen);
			send(app_sock, sendbuf, sendlen, 0);
			free(outbuf);
		} else {
			dprintf("Invalid output\n");
			ret = -4;
			break;
		}
	} while ( n < 3);

	if (sapbuf) free(sapbuf);
	close(app_sock);
	myprintf("Done\n");
}

int main()
{
	int listen_sock = 0;
	int app_sock = 0;
	struct sockaddr_in hostaddr;
	struct sockaddr_in clientaddr;
	int socklen = sizeof(clientaddr);

	pthread_t thread_id;
	pthread_attr_t attr;

	myprintf("Starting fairplay server\n");

#ifdef RUN_DAEMON
	if (daemon(0, 0) != 0) 
		myprintf("daemonlize failed!\n");
#endif

	memset((void *)&hostaddr, 0, sizeof(hostaddr));
	memset((void *)&clientaddr, 0, sizeof(clientaddr));

	hostaddr.sin_family = AF_INET;
	hostaddr.sin_port = htons(LISTEN_PORT);
	hostaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock < 0) {
		printf("%s:%d, create socket failed", __FILE__, __LINE__);
		exit(1);
	}
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
	if (bind(listen_sock, (struct sockaddr *)&hostaddr, sizeof(hostaddr)) <
	    0) {
		printf("%s:%d, bind socket failed", __FILE__, __LINE__);
		exit(1);
	}

	if (listen(listen_sock, MAX_LISTEN_NUM) < 0) {
		printf("%s:%d, listen failed", __FILE__, __LINE__);
		exit(1);
	}

	while (1) {
		app_sock =
		    accept(listen_sock, (struct sockaddr *)&clientaddr,
			   &socklen);
		if (app_sock < 0) {
			printf("%s:%d, accept failed", __FILE__, __LINE__);
			continue;
		} else {
			myprintf("accepted client from %s\n", inet_ntoa(clientaddr.sin_addr));
		}

		pthread_create(&thread_id, NULL, &handle_one_client,(void*)&app_sock);

	}
	close(listen_sock);

	return 0;
}
