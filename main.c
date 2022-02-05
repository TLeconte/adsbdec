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
extern int outmode;
extern int outformat;
extern char *filename;
extern int gain;
extern int df, errcorr;

extern int runOutput(void);
extern void handlerExit(int sig);

static void usage(void)
{
	printf("adsbdec airspy ADSB decoder 1.0 Copyright (c) 2018 Thierry Leconte  \n\n");
	printf("usage : adsbdec [-d] [-c] [-e] [-m] [-b] [-g 0-21 ] [-f filename] [-s addr[:port]] [-l addr[:port]]\n\n");
	printf
	    ("By default receive samples from airspy and output long adsb frames in raw avr format on stdout\n");
	printf("Options :\n");
	printf("\t-d : output short frames too\n");
	printf("\t-e : use 1 bit error correction\n");
	printf("\t-m : output avrmlat format (ie : with 12Mhz timestamp)\n");
	printf("\t-b : output binary beast format\n");
	printf("\t-g 0-21 : set linearity gain\n");
	printf("\t-f : input from filename instead of airspy (raw signed 16 bits real format)\n");
	printf ("\t-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)\n");
	printf ("\t-l addr[:port] : listen to addr:port (default port : 30002) and accept a TCP connection where to send output \n");

}

int main(int argc, char **argv)
{
	int c,res;
	struct sigaction sigact;


	while ((c = getopt(argc, argv, "cf:s:l:g:demb")) != EOF) {
		switch (c) {
		case 'f':
			filename = optarg;
			break;
		case 's':
			Rawaddr = optarg;
			outmode = 1 ;
			break;
		case 'l':
			Rawaddr = optarg;
			outmode = 2 ;
			break;
		case 'g':
			gain = atoi(optarg);
			break;
		case 'd':
			df = 1;
			break;
		case 'e':
			errcorr = 1;
			break;
		case 'm':
			outformat = 1;
			break;
		case 'b':
			outformat = 2;
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
