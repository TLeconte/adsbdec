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

extern int validShort(uint8_t *frame,const uint64_t ts,uint32_t pw);
extern int validLong(uint8_t *frame, const uint64_t ts,uint32_t pw);

static inline uint8_t getabyte(int idx) {
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


int deqframe(const int idx, const uint64_t sc)
{
	static uint32_t pv0, pv1, pv2;

	pv0 = pv1;
	pv1 = pv2;

	/* preamble power */
	pv2 =
	    ampbuff[idx] + ampbuff[idx + 2 * PULSEW] +
	     ampbuff[idx + 7 * PULSEW] + ampbuff[idx + 9 * PULSEW];

	/* peak detection */
	if (pv1 > pv0 && pv1 > pv2) {
		int lidx;
		uint32_t ns;
		uint8_t frame[14];
		int flen;
		int fmlen;

		lidx = idx - 1 ;

		/* noise estimation */
		ns = ampbuff[lidx + PULSEW] + ampbuff[lidx + 3 * PULSEW] + ampbuff[lidx + 4 * PULSEW] + ampbuff[lidx + 5 * PULSEW] + ampbuff[lidx + 6 * PULSEW] + ampbuff[lidx + 8 * PULSEW] ; 

		/* s/n test */
		if (pv1 < 2 * ns)  {
			return 1;
		}

		lidx+=16*PULSEW;

		/* decode 1st byte to get len */
		frame[0]=getabyte(lidx);

		switch(frame[0] >> 3) {
        		case 0: case 4: case 5: case 11:
				fmlen=7;
				if(df==0) {
					pv1=pv2=0;
					return 1;
				}
				break;
        		case 16: case 17: case 18: case 20: case 21: case 24:
				fmlen=14;
				break;
        		default:
				pv1=pv2=0;
				return 1;
    		}

		for(flen=1;flen<fmlen;flen++) {
			lidx+=16*PULSEW;
			frame[flen]=getabyte(lidx);
		}

		switch(fmlen) {
			case  7 :
				if (validShort(frame,sc,pv1)) {
					pv1=pv2=0;
					return 128 * PULSEW;
				}
			case 14 :
				if (validLong(frame,sc,pv1)) {
					pv1=pv2=0;
					return 240 * PULSEW;
				}
		}

	}
	return 1;
}


