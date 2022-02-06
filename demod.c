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

int deqframe(const int idx, const uint64_t sc)
{
	static uint32_t pv0, pv1, pv2;
	int lidx=idx;

	pv0 = pv1;
	pv1 = pv2;

	/* preamble power */
	pv2 =
	    ampbuff[idx] + ampbuff[idx + 2 * PULSEW] +
	     ampbuff[idx + 7 * PULSEW] + ampbuff[idx + 9 * PULSEW];

	/* peak detection */
	if (pv1 > pv0 && pv1 > pv2) 
	{
		uint8_t frame[14];
		int flen;
		uint8_t bits = 0;
		int fmlen;
		int k;
		int ns;

		lidx = idx - 1 ;

		/* noise estimation */
		ns = ampbuff[lidx + PULSEW] + ampbuff[lidx + 3 * PULSEW] + ampbuff[lidx + 8 * PULSEW] + ampbuff[lidx + 10 * PULSEW]; 

		/* s/n test */
		if (pv1 < 2*ns)  {
			return 1;
		}

		/* decode each bit */
		flen=0;fmlen=0;
		for (k = 0; k <= 14*8; k++) {

			if (k && k % 8 == 0) {
				frame[flen] = bits;
				flen++;

				if (flen == 1) {
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
				} 

				if (fmlen == 7 && flen == 7) {
					if (validShort(frame, sc, pv1)) {
						pv1=pv2=0;
						return 128 * PULSEW;
					}
				}

				if (fmlen == 14 && flen == 14) {
					if (validLong(frame,sc, pv1)) {
						pv1=pv2=0;
						return 240 * PULSEW;
					}
				}
				bits = 0;
			}

			bits = bits << 1;
			if (ampbuff[lidx + (16 + 2 * k) * PULSEW] >
			    ampbuff[lidx + (17 + 2 * k) * PULSEW])
				bits |= 1;
		}
	}
	return 1;
}


