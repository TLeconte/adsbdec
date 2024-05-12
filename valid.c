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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include "crc.h"

extern void netout(const uint8_t *frame, const int len, const uint64_t ts,const uint32_t pw);

int errcorr = 0;

static uint32_t stat_try[26] = { 0 };
static uint32_t stat_ok[26] = { 0 };
static uint32_t stat_ec[26] = { 0 };


static inline uint32_t icao(uint8_t *frame) 
{
	return (frame[1]<<16)|(frame[2]<<8)|frame[3];
}

int validShort(uint8_t *frame, const uint8_t type, const uint64_t ts, uint32_t pw)
{
	uint32_t crc=0;
	int n;

	stat_try[type]++;

	for(n=0;n<4;n++)
		crc=CrcStep(frame[n],crc);
	crc=CrcEnd(frame,crc,7);

	if (crc == 0 ) {
			stat_ok[type]++;
			netout(frame,7,ts,pw);
			return 1;
	}

	return 0;
}

int validLong(uint8_t *frame, const uint8_t type, const uint64_t ts, uint32_t pw)
{
	uint32_t crc=0;
	int n;

	stat_try[type]++;

	for(n=0;n<11;n++)
		crc=CrcStep(frame[n],crc);
	crc=CrcEnd(frame,crc,14);

        if(crc==0) {
		stat_ok[type]++;
		netout(frame, 14,ts,pw);
		return 1;
	
	}
	if(errcorr) {
		int nb;
		nb = testFix(frame, crc);
		if( nb >= 0 ) {
			stat_ec[type]++;
			fixChecksum(frame,nb);
			netout(frame, 14,ts,pw);
			return 1;
		}
	}

	return 0;
}

void print_stats(void)
{
	int tot_ok,tot_fi;

	fprintf(stderr,"\t%10d\t%10d\t%10d\n",11,17,18);
	fprintf(stderr,"Try :\t");
	fprintf(stderr,"%10d\t",stat_try[11]);
	fprintf(stderr,"%10d\t",stat_try[17]);
	fprintf(stderr,"%10d\n",stat_try[18]);

	fprintf(stderr,"Ok :\t");
	fprintf(stderr,"%10d\t",stat_ok[11]);
	fprintf(stderr,"%10d\t",stat_ok[17]);
	fprintf(stderr,"%10d\n",stat_ok[18]);
	tot_ok=stat_ok[11]+stat_ok[17]+stat_ok[18];

	fprintf(stderr,"ECC :\t");
	fprintf(stderr,"%10d\t",stat_ec[11]);
	fprintf(stderr,"%10d\t",stat_ec[17]);
	fprintf(stderr,"%10d\n",stat_ec[18]);
	tot_fi=stat_ec[11]+stat_ec[17]+stat_ec[18];

	fprintf(stderr,"Total :\t%10d\n",tot_ok+tot_fi);
}
