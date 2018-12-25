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
#include <libairspy/airspy.h>
#include "adsbdec.h"

#ifdef AIRSPY_MINI
#define AIR_SAMPLE_RATE 12000000
#else
#define AIR_SAMPLE_RATE 20000000
#endif

int gain = 21;

#define APBUFFSZ (1024*PULSEW)
uint32_t amp2buff[APBUFFSZ];

#define DECOFFSET (255*PULSEW)
static uint64_t timestamp;

extern void handlerExit(int sig);
extern int deqframe(const int idx, const uint64_t sc);

#ifdef AIRSPY_MINI
#define FILTERLEN 7
static const int pshape[FILTERLEN]={4,-4,-5,5,5,-4,-4};
#else
#define FILTERLEN 11
static const int pshape[FILTERLEN]={ -9,9,16,-16,-16,16,16,-16,-16,9,9 };
#endif

static void decodeiq(const short *r, const int len)
{
	static int inidx = 0;
	static int needsample = DECOFFSET;
	static int rbuff[FILTERLEN];

	int i,j;

	for (i = 0; i < len; i++) {
		int sumi,sumq;
		uint32_t off;

		off=timestamp%FILTERLEN;
		rbuff[off] = (int)r[i];
		timestamp++;

		sumi=sumq=0;
		off++;
		for(j=0;j<FILTERLEN-1;) {
			sumq+=rbuff[(off+j)%FILTERLEN]*pshape[j];j++;
			sumi+=rbuff[(off+j)%FILTERLEN]*pshape[j];j++;
		}
		sumq+=rbuff[(off+j)%FILTERLEN]*pshape[j];
                sumi/=16;sumq/=16;

		amp2buff[inidx++] = (sumi*sumi+sumq*sumq)/8;

		if(inidx==APBUFFSZ) {
			memcpy(amp2buff,&(amp2buff[APBUFFSZ-PULSEW-DECOFFSET]),(DECOFFSET+PULSEW)*sizeof(int));
			inidx=DECOFFSET+PULSEW;
		}	

		needsample--;
		if (needsample == 0)
			needsample = deqframe(inidx-DECOFFSET, timestamp );
	}
}


static struct airspy_device *device = NULL;

int initAirspy(void)
{
	int result;
	uint32_t i, count;
	uint32_t *supported_samplerates;
	struct timespec tp;

	/* init airspy */
	result = airspy_open(&device);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_open() failed: %s (%d)\n", airspy_error_name(result),
			result);
		return -1;
	}

	result = airspy_set_sample_type(device, AIRSPY_SAMPLE_INT16_REAL);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_set_sample_type() failed: %s (%d)\n",
			airspy_error_name(result), result);
		airspy_close(device);
		return -1;
	}

	result = airspy_set_packing(device, 1);
	airspy_get_samplerates(device, &count, 0);
	supported_samplerates = (uint32_t *) malloc(count * sizeof(uint32_t));
	airspy_get_samplerates(device, supported_samplerates, count);
	for (i = 0; i < count; i++) {
		if (supported_samplerates[i] == AIR_SAMPLE_RATE)
			break;
	}
	if (i >= count) {
		fprintf(stderr, "did not find needed sampling rate\n");
		airspy_close(device);
		return -1;
	}
	free(supported_samplerates);

	result = airspy_set_samplerate(device, i);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_set_samplerate() failed: %s (%d)\n",
			airspy_error_name(result), result);
		airspy_close(device);
		return -1;
	}

	/* had problem with packing , disable it */
	airspy_set_packing(device, 0);

	result = airspy_set_linearity_gain(device, gain);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_set_linearity_gain() failed: %s (%d)\n",
			airspy_error_name(result), result);
		airspy_close(device);
		return -1;
	}

	result = airspy_set_freq(device, 1090000000);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_set_freq() failed: %s (%d)\n", airspy_error_name(result),
			result);
		airspy_close(device);
		return -1;
	}

       /* reduce FI bandwidth */
        airspy_r820t_write(device, 10, 0xB0 | 15);
        airspy_r820t_write(device, 11, 0x0 | 6 );

	/* just to init timestamp to something */
       	clock_gettime(CLOCK_REALTIME, &tp);
#ifdef AIRSPY_MINI
	timestamp=tp.tv_sec*12000000LL+tp.tv_nsec/83; /* 12Mb/s clock */
#else
	timestamp=tp.tv_sec*20000000LL+tp.tv_nsec/50; /* 20Mb/s clock */
#endif

	return 0;
}

static int rx_callback(airspy_transfer_t * transfer)
{
	decodeiq((short *)(transfer->samples), transfer->sample_count);
	return 0;
}

int startAirspy(void)
{
	int result;

        if(device == NULL) return 0;

        if(airspy_is_streaming(device) == AIRSPY_TRUE)
		return 0;

	result = airspy_start_rx(device, rx_callback, NULL);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_start_rx() failed: %s (%d)\n", airspy_error_name(result),
			result);
		airspy_close(device);
		return -1;
	}

	return 0;
}

void stopAirspy(void)
{
  if(device == NULL) return ;

  airspy_stop_rx(device);
}


void closeAirspy(void)
{

   if(device == NULL) return ;

    airspy_stop_rx(device);
    airspy_close(device);

}

char *filename = NULL;
#define IQBUFFSZ (1024*1024)
void *fileInput(void *arg)
{
	int fd;
	short *iqbuff;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		handlerExit(0);
		return NULL;
	}

	iqbuff = (short *)malloc(IQBUFFSZ * sizeof(short));

	do {
		int n;

		n = read(fd, iqbuff, IQBUFFSZ * sizeof(short));
		if (n <= 0)
			break;

		decodeiq(iqbuff, n / 2);

	} while (1);

	close(fd);
	handlerExit(0);
	return NULL;
}

