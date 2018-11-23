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
#define AIR_SAMPLE_RATE 12000000
#else
#define AIR_SAMPLE_RATE 20000000
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
