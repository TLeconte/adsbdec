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

#define AIR_SAMPLE_RATE 20000000
int gain = 18;

#define APBUFFSZ (8196*PULSEW)
#define FILTERLEN 8 

static float ampbuff[APBUFFSZ];
static uint32_t inidx = 0;
static uint32_t fidx = 0 ;
static int rbuff[FILTERLEN];

extern void handlerExit(int sig);

static void decodeiq(const unsigned short *r, const int len)
{

	int rlen;

	if(len&1) fprintf(stderr,"odd input buffer len\n");

	for (int i = 0; i < len; ) {
		int sumi,sumq;

		/* downsample by 2 */
		rbuff[fidx%FILTERLEN] = (int)r[i]-0x800;i++;
		fidx++;
		rbuff[fidx%FILTERLEN] = (int)r[i]-0x800;i++;
		fidx++;

		/* box filter + fs/4 mixing  unrolled */
		sumi=sumq=0;
		sumq+=rbuff[0];
		sumi+=rbuff[1];
		sumq-=rbuff[2];
		sumi-=rbuff[3];
		sumq+=rbuff[4];
		sumi+=rbuff[5];
		sumq-=rbuff[6];
		sumi-=rbuff[7];

		/* iq to power */
		ampbuff[inidx]=(sumi*sumi+sumq*sumq);
		inidx++;

		if(inidx==APBUFFSZ) {
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


static struct airspy_device *device = NULL;

int initSDR(void)
{
	int result;
	uint32_t i, count;
	uint32_t *supported_samplerates;

	/* init airspy */
	result = airspy_open(&device);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_open() failed: %s (%d)\n", airspy_error_name(result),
			result);
		return -1;
	}

	result = airspy_set_sample_type(device, AIRSPY_SAMPLE_RAW);
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

	return 0;
}

static int rx_callback(airspy_transfer_t * transfer)
{
	decodeiq((unsigned short *)(transfer->samples), transfer->sample_count);
	return 0;
}

int startSDR(void)
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

void stopSDR(void)
{
  if(device == NULL) return ;

  airspy_stop_rx(device);
}


void closeSDR(void)
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
	unsigned short *iqbuff;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		handlerExit(0);
		return NULL;
	}

	iqbuff = (unsigned short *)malloc(IQBUFFSZ * sizeof(short));

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

