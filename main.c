#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

extern char *Rawaddr;
extern int gain;

char *filename = NULL;

extern int initAirspy(void);
extern int runAirspySample(void);

extern int df, errcorr;
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
	printf("usage : adsbdec [-d] [-e] [-f filename] [-s addr[:port]]\n\n");
	printf
	    ("By default receive samples from airspy and output DF11/17/18 adsb frames in raw avr format on stdout\n");
	printf("Options :\n");
	printf("\t-d : don't output DF11 frames\n");
	printf("\t-e : don't use 1bit error correction\n");
	printf("\t-f : input from filename instead of airspy (raw signed 16 bits IQ format)\n");
	printf
	    ("\t-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)\n");
	printf("\nExample :\n");
	printf("\tOn computer connected to airspy :\n");
	printf("\t\tadsbdec -s 192.168.0.10:30001\n");
	printf("\tOn another computer at addr 192.168.0.10 :\n");
	printf("\t\tdump1090 --net-only --net-ri-port 30001 \n");
	printf
	    ("\nMan could use adsbdec to send data to any other avr format compatible server (VRS, feeders for main adsb web site, etc )\n\n");

}

int main(int argc, char **argv)
{
	int c;
	struct sigaction sigact;

	while ((c = getopt(argc, argv, "f:s:g:de")) != EOF) {
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
		case 'e':
			errcorr = 0;
			break;
		default:
			usage();
			return 1;
		}
	}

	sigact.sa_handler = SIG_IGN;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGPIPE, &sigact, NULL);

	if (filename) {
		fileInput(filename);
	} else {
		initAirspy();
		runAirspySample();
	}

	return 0;
}
