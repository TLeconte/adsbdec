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
#include <libairspy/airspy.h>

#ifdef AIRSPY_MINI
#define AIR_SAMPLE_RATE 6000000
#else
#define AIR_SAMPLE_RATE 10000000
#endif

int gain = 21;

extern void decodeiq(short *iq, int len);

static struct airspy_device *device = NULL;

int initAirspy(void)
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

	result = airspy_set_sample_type(device, AIRSPY_SAMPLE_INT16_IQ);
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
	for (i = 0; i < count; i++)
		if (supported_samplerates[i] == AIR_SAMPLE_RATE)
			break;
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

	result = airspy_set_packing(device, 1);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_set_packing true failed: %s (%d)\n",
			airspy_error_name(result), result);
	}

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
	decodeiq((short *)(transfer->samples), transfer->sample_count);
	return 0;
}

int runAirspySample(void)
{
	int result;

	result = airspy_start_rx(device, rx_callback, NULL);
	if (result != AIRSPY_SUCCESS) {
		fprintf(stderr, "airspy_start_rx() failed: %s (%d)\n", airspy_error_name(result),
			result);
		airspy_close(device);
		return -1;
	}

	while (airspy_is_streaming(device) == AIRSPY_TRUE) {
		sleep(2);
	}

	airspy_close(device);
	return 0;
}

void stopAirspy(int sig)
{
	if(device)
		 airspy_stop_rx(device);
	else
		exit(1);
}
