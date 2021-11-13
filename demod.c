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
#include "crc.h"
#include "adsbdec.h"

int df = 0;

extern int validShort(uint8_t *frame,const uint64_t ts,uint32_t crc,u_char lvl);
extern int validLong(uint8_t *frame, const uint64_t ts,uint32_t crc,u_char lvl);


#define APBUFFSZ (1024*PULSEW)
extern uint32_t amp2buff[APBUFFSZ];

int deqframe(const int idx, const uint64_t sc)
{
	static uint32_t pv0, pv1, pv2;
	int lidx=idx;

	pv0 = pv1;
	pv1 = pv2;

	/* preamble power */
	pv2 =
	    amp2buff[idx] + amp2buff[idx + 2 * PULSEW] +
	     amp2buff[idx + 7 * PULSEW] + amp2buff[idx + 9 * PULSEW];

	/* peak detection */
	if (pv1 > pv0 && pv1 > pv2) 
	{
		uint8_t frame[14];
		uint32_t crc;
		int flen = 0;
		uint8_t bits = 0;
		int k;
		int ns;
		u_char lvl;

		if(pv1 > 255<<13) lvl=255; else lvl=pv1>>13;

		lidx = idx - 1 ;

		/* noise estimation */
		ns = amp2buff[lidx + PULSEW] + amp2buff[lidx + 3 * PULSEW] + amp2buff[lidx + 8 * PULSEW] + amp2buff[lidx + 10 * PULSEW]; 

		/* s/n test */
		if ( pv1 < 2 * ns)  {
			return 1;
		}

		/* decode each bit */
		for (k = 0; k < 112; k++) {

			if (k && k % 8 == 0) {
				frame[flen] = bits;
				bits = 0;
				flen++;

				if (flen == 7) {
					crc=CrcShort(frame);
					if (df && validShort(frame, sc, CrcEnd(frame,crc,7),lvl)) {
						pv1=pv2=0;
						return 128 * PULSEW;
					}
				}
			}

			bits = bits << 1;
			if (amp2buff[lidx + (16 + 2 * k) * PULSEW] >
			    amp2buff[lidx + (17 + 2 * k) * PULSEW])
				bits |= 1;
		}
		frame[flen] = bits;
		flen++;
		crc=CrcLong(frame,crc);
		if (validLong(frame,sc, CrcEnd(frame,crc,14),lvl)) {
			pv1=pv2=0;
			return 240 * PULSEW;
		}
	}
	return 1;
}


