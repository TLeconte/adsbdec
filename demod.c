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
#include <math.h>
#include "adsbdec.h"

int df = 0;

extern int validShort(uint8_t *frame, const uint8_t type, const uint64_t ts,uint32_t pw);
extern int validLong(uint8_t *frame, const uint8_t type, const uint64_t ts,uint32_t pw);

static inline uint8_t getabyte(const float *ampbuff, const int idx) {
	uint8_t bits=0;

	if (ampbuff[idx] > ampbuff[idx + PULSEW]) bits |= 0x80;
	if (ampbuff[idx + 2 * PULSEW] > ampbuff[idx + 3 * PULSEW]) bits |= 0x40;
	if (ampbuff[idx + 4 * PULSEW] > ampbuff[idx + 5 * PULSEW]) bits |= 0x20;
	if (ampbuff[idx + 6 * PULSEW] > ampbuff[idx + 7 * PULSEW]) bits |= 0x10;
	if (ampbuff[idx + 8 * PULSEW] > ampbuff[idx + 9 * PULSEW]) bits |= 0x08;
	if (ampbuff[idx + 10 * PULSEW] > ampbuff[idx + 11 * PULSEW]) bits |= 0x04;
	if (ampbuff[idx + 12 * PULSEW] > ampbuff[idx + 13 * PULSEW]) bits |= 0x02;
	if (ampbuff[idx + 14 * PULSEW] > ampbuff[idx + 15 * PULSEW]) bits |= 0x01;

	return bits;
}

#define SN 2
int deqframe(const float *ampbuff, const int len)
{
	static uint64_t ts=0;
	int idx;

	for(idx = 0; idx< len-DECOFFSET; ) {

	int p1,p2;
	int s1,s2;
	int lidx;
	float ns;
	uint8_t frame[14];
	uint8_t type;
	int flen;
	int fmlen;

	ts++;

	/* preamble detection */
	p1 = ampbuff[idx] + ampbuff[idx + 2 * PULSEW];
	s1 = ampbuff[idx + PULSEW] + ampbuff[idx + 3 * PULSEW];
        p2 = ampbuff[idx + 7 * PULSEW] + ampbuff[idx + 9 * PULSEW];
	s2 = ampbuff[idx + 6 * PULSEW] + ampbuff[idx + 8 * PULSEW];
	if( p1 > SN * s1 && p2 > SN * s2){

		lidx=idx+16*PULSEW;

		/* decode 1st byte to get len */
		frame[0]=getabyte(ampbuff,lidx);

		type = frame[0] >> 3 ;

		switch(type) {
		       	case 11:
				fmlen=7;
				break;
        		case 17: case 18:
				fmlen=14;
				if(df==0) {
					idx++;
					continue;
				}
				break;
        		default:
				idx++;
				continue;
    		}

		for(flen=1;flen<fmlen;flen++) {
			lidx+=16*PULSEW;
			frame[flen]=getabyte(ampbuff,lidx);
		}
		lidx+=16*PULSEW;

		switch(fmlen) {
			case  7 :
				if (validShort(frame,type,ts,(p1+p2)/4)) {
					idx= lidx ;
					continue;
				}
				break;
			case 14 :
				if (validLong(frame,type,ts,(p1+p2)/4)) {
					idx= lidx ;
					continue;
				}
				break;
		}

	}
	idx++;
	}
	return idx;	
}


