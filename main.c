/*
 *  Copyright (c) 2018 Thierry Leconte
 *
 *   
 *   This code is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

extern char *Rawaddr;
extern int outformat;

extern int gain;
extern unsigned long int timestamp;

char *filename = NULL;

extern int initAirspy(void);
extern void stopAirspy(int sig);
extern int runAirspySample(void);

extern int df, errcorr,crcok;
extern void decodeiq(short *iq, int len);

#define IQBUFFSZ (1024*1024)
static int fileInput(char *filename)
{
	int fd;
	short *iqbuff;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return -2;

	iqbuff = (short *)malloc(IQBUFFSZ * sizeof(short));

	do {
		int n;

		n = read(fd, iqbuff, IQBUFFSZ * sizeof(short));
		if (n <= 0)
			break;

		decodeiq(iqbuff, n / 2);

	} while (1);

	close(fd);
	return 0;
}

static void usage(void)
{
	printf("adsbdec airspy ADSB decoder Copyright (c) 2018 Thierry Leconte  \n\n");
	printf("usage : adsbdec [-d] [-e] [-m] [-f filename] [-s addr[:port]]\n\n");
	printf
	    ("By default receive samples from airspy and output DF17/18 adsb frames in raw avr format on stdout\n");
	printf("Options :\n");
	printf("\t-d : output short frames too\n");
	printf("\t-c : only frame wit true crc (DF11/17/18)\n");
	printf("\t-e : use 1bit error correction (more packets, more CPU, more false packets)\n");
	printf("\t-m : output avrmlat format (ie : with 12Mhz timestamp)\n");
	printf("\t-f : input from filename instead of airspy (raw signed 16 bits IQ format)\n");
	printf
	    ("\t-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)\n");
	printf("\nExample :\n");
	printf("\t\tadsbdec -e -d -c -s 192.168.0.10:30001\n");
	printf
	    ("\nMan could use adsbdec to send data to any other avr format compatible server (VRS, feeders for main adsb web site, etc )\n\n");

}

int main(int argc, char **argv)
{
	int c;
	struct sigaction sigact;
	struct timespec tp;

	while ((c = getopt(argc, argv, "cf:s:g:dem")) != EOF) {
		switch (c) {
		case 'f':
			filename = optarg;
			break;
		case 's':
			Rawaddr = optarg;
			break;
		case 'g':
			gain = atoi(optarg);
			break;
		case 'd':
			df = 1;
			break;
		case 'c':
			crcok = 1;
			break;
		case 'e':
			errcorr = 1;
			break;
		case 'm':
			/* just to init timestamp to something */
       			clock_gettime(CLOCK_REALTIME, &tp);
#ifdef AIRSPY_MINI
			timestamp=tp.tv_sec*12000000LL+tp.tv_nsec/83; /* 12Mb/s clock */
#else
			timestamp=tp.tv_sec*20000000LL+tp.tv_nsec/50; /* 20Mb/s clock */
#endif

			outformat = 1;
			break;
		default:
			usage();
			return 1;
		}
	}

	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = stopAirspy;
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigact.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigact, NULL);

	if (filename) {
		fileInput(filename);
	} else {
		if(initAirspy()<0) return -1;
		runAirspySample();
	}

	return 0;
}
