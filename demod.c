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

int df = 0;

extern void netout(const uint8_t *frame, const int len, const uint64_t ts);
extern int validframe(uint8_t *frame, const int len);

#ifdef AIRSPY_MINI
#define PULSEW 6
#else
#define PULSEW 10
#endif

#define APBUFFSZ (1024*PULSEW)
static uint32_t amp2buff[APBUFFSZ];

static inline int deqframe(const int idx, const uint64_t sc)
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
		int flen = 0;
		uint8_t bits = 0;
		int k;
		int ns;
		uint64_t ts = 0;

		lidx = idx - 1 ;

		/* noise estimation */
		ns = 2*(amp2buff[lidx + PULSEW] + amp2buff[lidx + 8 * PULSEW]); 

		/* s/n test */
		if ( pv1 < 2 *  ns)  {
			return 1;
		}

		ts = 12*((sc - 1)&0xfffffffffffffff)/10;

		/* decode each bit */
		for (k = 0; k < 112; k++) {

			if (k && k % 8 == 0) {
				frame[flen] = bits;
				bits = 0;
				flen++;

				if (df && flen == 7) {
					if (validframe(frame, flen)) {
						netout(frame, flen, ts);
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
		if (validframe(frame, flen)) {
			netout(frame, flen, ts);
			pv1=pv2=0;
			return 240 * PULSEW;
		}
	}
	return 1;
}

#define DECOFFSET (255*PULSEW)
uint64_t samplecount = 0;

#ifdef AIRSPY_MINI
const int pshape[2*PULSEW]={4,5,4,4,5,4};
#else
const int pshapeI[2*PULSEW]={3,3,-4,-4,4,4,-4,-4,3,3,3,3,-4,-4,4,4,-4,-4,3,3};
const int pshapeQ[2*PULSEW]={3,-3,-4,4,4,-4,-4,4,3,-3,3,-3,-4,4,4,-4,-4,4,3,-3};
#endif

void decodeiq(const short *r, const int len)
{
	static int inidx = 0;
	static int needsample = DECOFFSET;
	static int rbuff[PULSEW];

	int i,j;

	for (i = 0; i < len; i++) {
		int sumi,sumq;
		uint32_t off;

		off=samplecount%PULSEW;
		rbuff[off] = (int)r[i];
		samplecount++;

		sumi=sumq=0;
		for(j=0;j<PULSEW;j++) {
			sumi+=rbuff[j]*pshapeI[j+PULSEW-1-off];
			sumq+=rbuff[j]*pshapeQ[j+PULSEW-1-off];
		}
		amp2buff[inidx++] = sumi*sumi+sumq*sumq;

		if(inidx==APBUFFSZ) {
			memcpy(amp2buff,&(amp2buff[APBUFFSZ-PULSEW-DECOFFSET]),(DECOFFSET+PULSEW)*sizeof(int));
			inidx=DECOFFSET+PULSEW;
		}	

		needsample--;
		if (needsample == 0)
			needsample = deqframe(inidx-DECOFFSET, samplecount );
	}
}
