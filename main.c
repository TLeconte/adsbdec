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
#include <signal.h>


extern char *Rawaddr;
extern int outformat;
extern char *filename;
extern int gain;
extern int df, errcorr,crcok;

extern int runOutput(void);
extern void handlerExit(int sig);

static void usage(void)
{
	printf("adsbdec airspy ADSB decoder 1.0 Copyright (c) 2018 Thierry Leconte  \n\n");
	printf("usage : adsbdec [-d] [-c] [-e] [-m] [-g 0-21 ] [-f filename] [-s addr[:port]]\n\n");
	printf
	    ("By default receive samples from airspy and output long adsb frames in raw avr format on stdout\n");
	printf("Options :\n");
	printf("\t-d : output short DF11 frames too\n");
	printf("\t-c : only frame with true crc (DF11/17/18)\n");
	printf("\t-e : use 1 bit error correction\n");
	printf("\t-m : output avrmlat format (ie : with 12Mhz timestamp)\n");
	printf("\t-g 0-21 : set linearity gain (default 21)\n");
	printf("\t-f : input from filename instead of airspy (raw signed 16 bits real format)\n");
	printf
	    ("\t-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)\n");
	printf("\nExample :\n");
	printf("\t\tadsbdec -e -d -c -s 192.168.0.10:30001\n");
	printf
	    ("\nMan could use adsbdec to send data to any avr format compatible server (VRS, feeders for main adsb web site, etc )\n\n");

}

int main(int argc, char **argv)
{
	int c,res;
	struct sigaction sigact;


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
			outformat = 1;
			break;
		default:
			usage();
			return 1;
		}
	}

	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = handlerExit;
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	sigact.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigact, NULL);

	res = runOutput();

	return res;
}
