#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

char *Rawaddr = NULL;

static int sockfd = -1;

static int initNet(void)
{
	char *addr;
	char *port;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	if (Rawaddr[0] == '[') {
		hints.ai_family = AF_INET6;
		addr = Rawaddr + 1;
		port = strstr(addr, "]");
		if (port == NULL) {
			fprintf(stderr, "Invalid IPV6 address\n");
			return -1;
		}
		*port = 0;
		port++;
		if (*port != ':')
			port = "30001";
		else
			port++;
	} else {
		hints.ai_family = AF_UNSPEC;
		addr = Rawaddr;
		port = strstr(addr, ":");
		if (port == NULL)
			port = "30001";
		else {
			*port = 0;
			port++;
		}
	}

	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "Invalid/unknown address %s\n", addr);
		return -1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "failed to connect\r");
		return -1;
	}
	fprintf(stderr, "connected                    \n");

	freeaddrinfo(servinfo);

	return 0;
}

void netout(unsigned char *frame, int len)
{
	char pkt[50];
	int i, res;

	sprintf(pkt, "*");
	for (i = 0; i < len; i++) {
		sprintf(&(pkt[2 * i + 1]), "%02X", frame[i]);

	}
	strcat(pkt, ";\n");

	if (Rawaddr) {
		if (sockfd < 0)
			initNet();
		if (sockfd < 0)
			return;
		res = write(sockfd, pkt, strlen(pkt));
		if (res <= 0) {
			close(sockfd);
			sockfd = -1;
		}
	} else
		fwrite(pkt, strlen(pkt), 1, stdout);

}
