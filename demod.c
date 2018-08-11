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
#include <math.h>
#include <inttypes.h>

int df = 1;
int errcorr = 1;
int outformat = 0;

extern uint32_t modesChecksum(const unsigned char *message, const int n);
extern uint32_t fixChecksum(unsigned char *message, const uint32_t ecrc, const int n);

extern void netout(const unsigned char *frame, const int len, const int outformat, const unsigned long int ts);

#define PULSEW 5
#define APBUFFSZ (1024*1024)
static uint32_t ampbuff[APBUFFSZ];

static inline uint32_t pulseamp(const int idx)
{
	 int A;
	 uint32_t *v=&(ampbuff[idx]);

	A = v[0]/2 + v[1] + v[2] + v[3] + v[4]/2;
	return A;
}

#if 0
static void outpulse(int idx)
{
	int i;
	int *v=&(ampbuff[idx]);
	float A;

	A = v[0] + v[1] + v[2] + v[3] + v[4];

	for (i = -1; i < 10; i++) {
			printf("%d %f\n", i, (float) ampbuff[idx + i ] / A);
	}
	printf("\n\n");

}
#endif

static int validframe(unsigned char *frame, const int len, const unsigned long int ts)
{
	uint32_t crc;
	uint32_t type;
	int r;

	type = frame[0] >> 3;
	if ((type != 17 && type != 18 && type != 11) || ((type == 17 || type == 18) && len != 14)
	    || (type == 11 && len != 7)
	    )
		return 0;

	if (frame[len - 3] == 0 && frame[len - 2] == 0 && frame[len - 1] == 0)
		return 0;

	crc = modesChecksum(frame, len);

	if (type == 11) {
		if (crc & 0xffff80) {
			if (errcorr)
				r = fixChecksum(frame, crc, len);
			else
				r = 0;
			if (r == 0)
				return 0;
		}
	} else {
		if (crc) {
			if (errcorr)
				r = fixChecksum(frame, crc, len);
			else
				r = 0;
			if (r == 0)
				return 0;
		}
	}

	netout(frame, len, outformat, ts);

	return 1;
}

static int deqframe(const int idx, const unsigned long int sc)
{
	static uint32_t pv0, pv1, pv2;
	int lidx=idx;

	pv0 = pv1;
	pv1 = pv2;

	/* preamble matched filter */
	pv2 =
	    pulseamp(idx) + pulseamp(idx + 2 * PULSEW) +
	     pulseamp(idx + 7 * PULSEW) + pulseamp(idx + 9 * PULSEW);

	/* peak detection */
	if (pv1 > pv0 && pv1 > pv2) 
	{
		unsigned char frame[14];
		int flen = 0;
		unsigned char bits = 0;
		int k;
		int ns;
		unsigned long int ts = 0;

		lidx = idx - 1 ;

		/* noise estimation */
		ns = 2*(pulseamp(lidx + PULSEW) + pulseamp(lidx + 8 * PULSEW)); 

		/* s/n test */
		if (2 * pv1 < 3 * ns)  {
			return 1;
		}

		if (outformat)
			ts = 12*(sc - 1)/10;

		/* decode each bit */
		for (k = 0; k < 112; k++) {

			if (k && k % 8 == 0) {
				frame[flen] = bits;
				bits = 0;
				flen++;

				if (df && flen == 7) {
					if (validframe(frame, flen, ts)) {
						pv1=pv2=0;
						return 128 * PULSEW;
					}
				}
			}

			bits = bits << 1;
			if (pulseamp(lidx + (16 + 2 * k) * PULSEW) >
			    pulseamp(lidx + (17 + 2 * k) * PULSEW))
				bits |= 1;
		}
		frame[flen] = bits;
		flen++;
		if (validframe(frame, flen, ts)) {
			pv1=pv2=0;
			return 240 * PULSEW;
		}
	}
	return 1;
}

#define DECOFFSET (255*PULSEW)
unsigned long int samplecount = 0;
void decodeiq(const short *iq, const int len)
{
	static int inidx = 0;
	static int needsample = DECOFFSET;

	int i;

	for (i = 0; i < len; i += 2) {
		int iv,qv;
		uint32_t amp;

		iv = (int)(iq[i]);
		qv = (int)(iq[i + 1]);
		amp = qv*qv+iv*iv;

		ampbuff[inidx] = amp;
		inidx++;
		if(inidx==APBUFFSZ) {
			memcpy(ampbuff,&(ampbuff[APBUFFSZ-PULSEW-DECOFFSET]),(DECOFFSET+PULSEW)*sizeof(int));
			inidx=DECOFFSET+PULSEW;
		}	

		samplecount++;

		needsample--;
		if (needsample == 0)
			needsample = deqframe(inidx-DECOFFSET, samplecount );
	}

}
