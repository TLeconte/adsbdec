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
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <rtl-sdr.h>
#include "adsbdec.h"

int gain = 493;

#define SDRINRATE  2400000
static rtlsdr_dev_t *dev = NULL;

#define IQBUFFSZ (128*1024)
#define APBUFFSZ (4096*PULSEW)
static float ampbuff[APBUFFSZ+5];
static uint32_t inidx = 0;

static void in_callback(unsigned char *r, uint32_t len, void *ctx)
{
        static float pi,pq;

        int rlen;

        if(len&1) fprintf(stderr,"odd input buffer len\n");

        for (int i = 0; i < len; ) {
                float ci,cq;
                float vi,vq;

                ci=(float)r[i++]-128.5;
                cq=(float)r[i++]-128.5;

                vi=0.9045*pi+0.0955*ci;
                vq=0.9045*pq+0.0955*cq;
                ampbuff[inidx++]=(vi*vi+vq*vq);

                vi=0.6545*pi+0.3455*ci;
                vq=0.6545*pq+0.3455*cq;
                ampbuff[inidx++]=(vi*vi+vq*vq);

                vi=0.3455*pi+0.6545*ci;
                vq=0.3455*pq+0.6545*cq;
                ampbuff[inidx++]=(vi*vi+vq*vq);

                vi=0.0955*pi+0.9045*ci;
                vq=0.0955*pq+0.9045*cq;
                ampbuff[inidx++]=(vi*vi+vq*vq);

                ampbuff[inidx++]=(ci*ci+cq*cq);

		pi=ci;pq=cq;

                if(inidx>=APBUFFSZ) {
                        rlen = deqframe(ampbuff, inidx);
                        if(rlen<inidx)
                                memcpy(ampbuff,&(ampbuff[rlen]),(inidx-rlen)*sizeof(float));
                        inidx=inidx-rlen;
                }
        }
        rlen = deqframe(ampbuff, inidx);
        if(rlen<inidx)
                memcpy(ampbuff,&(ampbuff[rlen]),(inidx-rlen)*sizeof(float));
        inidx=inidx-rlen;
}


/* function verbose_device_search by Kyle Keen
 * from http://cgit.osmocom.org/rtl-sdr/tree/src/convenience/convenience.c
 */
static int verbose_device_search(char *s)
{
	int i, device_count, device, offset;
	char *s2;
	char vendor[256], product[256], serial[256];
	device_count = rtlsdr_get_device_count();
	if (!device_count) {
		fprintf(stderr, "No supported devices found.\n");
		return -1;
	}
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
	}
	/* does string look like raw id number */
	device = (int)strtol(s, &s2, 0);
	if (s2[0] == '\0' && device >= 0 && device < device_count) {
		return device;
	}
	/* does string exact match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strcmp(s, serial) != 0) {
			continue;
		}
		device = i;
		return device;
	}
	/* does string prefix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strncmp(s, serial, strlen(s)) != 0) {
			continue;
		}
		device = i;
		return device;
	}
	/* does string suffix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		offset = strlen(serial) - strlen(s);
		if (offset < 0) {
			continue;
		}
		if (strncmp(s, serial + offset, strlen(s)) != 0) {
			continue;
		}
		device = i;
		return device;
	}
	fprintf(stderr, "No matching devices found.\n");
	return -1;
}

static int nearest_gain(int target_gain)
{
	int i, err1, err2, count, close_gain;
	int *gains;
	count = rtlsdr_get_tuner_gains(dev, NULL);
	if (count <= 0) {
		return 0;
	}
	gains = malloc(sizeof(int) * count);
	count = rtlsdr_get_tuner_gains(dev, gains);
	close_gain = gains[0];
	for (i = 0; i < count; i++) {
		err1 = abs(target_gain - close_gain);
		err2 = abs(target_gain - gains[i]);
		if (err2 < err1) {
			close_gain = gains[i];
		}
	}
	free(gains);
	fprintf(stderr, "Gain:%d\n",close_gain);
	return close_gain;
}

int initSDR(void)
{
	int r;
	int dev_index;

	/*dev_index = verbose_device_search(argv[optind]); */
	dev_index = 0 ;

	r = rtlsdr_open(&dev, dev_index);
	if (r < 0) {
		fprintf(stderr, "Failed to open rtlsdr device\n");
		return r;
	}

	rtlsdr_set_tuner_gain_mode(dev, 1);
	r = rtlsdr_set_tuner_gain(dev, nearest_gain(gain));
	if (r < 0)
		fprintf(stderr, "WARNING: Failed to set gain.\n");

	r = rtlsdr_set_center_freq(dev, 1090000000);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to set center freq.\n");
		return 1;
	}

	r = rtlsdr_set_sample_rate(dev, SDRINRATE);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to set sample rate.\n");
		return 1;
	}

	r = rtlsdr_reset_buffer(dev);
	if (r < 0) {
		fprintf(stderr, "WARNING: Failed to reset buffers.\n");
		return 1;
	}

	return 0;
}

static void *readthread(void *arg)
{
	int r=-1;

	if(dev) r = rtlsdr_read_async(dev, in_callback, NULL, 4, IQBUFFSZ);
	if (r) {
		fprintf(stderr, "Read async %d\n", r);
	}
	return NULL;
}

int startSDR(void)
{
       pthread_t th;
	fprintf(stderr, "StartSDR\n");
        pthread_create(&th, NULL, readthread, NULL);
	return 0;
}

void stopSDR(void)
{
	fprintf(stderr, "StopSDR\n");
	if(dev) rtlsdr_cancel_async(dev);
}


void closeSDR(void)
{
  if (dev) {
     rtlsdr_close(dev);
     dev = NULL;
  }
}

char *filename = NULL;
void *fileInput(void *arg)
{
	int fd;
	unsigned char *iqbuff;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		handlerExit(0);
		return NULL;
	}

	iqbuff = (unsigned char *)malloc(IQBUFFSZ * sizeof(unsigned char));

	do {
		int n;

		n = read(fd, iqbuff, IQBUFFSZ * sizeof(unsigned char));
		if (n <= 0)
			break;

		in_callback(iqbuff, n, NULL);

	} while (1);

	close(fd);
	handlerExit(0);
	return NULL;
}

