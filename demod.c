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

int df = 1;
int errcorr = 1;
int outformat = 0;

extern unsigned int modesChecksum(const unsigned char *message, const int n);
extern unsigned int fixChecksum(unsigned char *message, const unsigned int ecrc, const int n);

extern void netout(const unsigned char *frame, const int len, const int outformat, const unsigned long int ts);

#define PULSEW 5
#define APBUFFSZ (1024*1024)
static float ampbuff[APBUFFSZ];

static inline float pulseamp(const int idx, const int shp)
{
	float A;
	float *v=&(ampbuff[idx]);

	if (shp)
		A = v[0] + 1.25 * v[1] + 1.18 * v[2] + 1.1 * v[3] + 0.96 * v[4];
	else
		A = 1.098 * (v[0] + v[1] + v[2] + v[3] + v[4]);
	return A;
}

#if 0
static void outpulse(int idx)
{
	int i;
	float *v=&(ampbuff[idx]);
	float A;

	A = v[0] + v[1] + v[2] + v[3] + v[4];

	for (i = -1; i < 10; i++)
		printf("%d %f\n", i, 5 * ampbuff[(idx + i + APBUFFSZ) % APBUFFSZ] / A);
	printf("\n\n");

}
#endif

static int validframe(unsigned char *frame, const int len, const unsigned long int ts)
{
	unsigned int crc;
	unsigned int type;
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

static inline unsigned long int computetimestamp(const float pv0, const float pv1, const float pv2, const unsigned long int sc)
{
	double offset, tsf;

	offset = (pv0 - pv2) / (pv0 + pv2 - 2 * pv1) / 2.0;
	tsf = 12.0 * ((double)sc + offset) / 10.0;

	return (unsigned long int)tsf;
}

static int deqframe(const int idx, const unsigned long int sc)
{
	static float pv0, pv1, pv2;
	int lidx=idx;

	pv0 = pv1;
	pv1 = pv2;

	/* preamble matched filter */
	pv2 =
	    pulseamp(idx, 1) + pulseamp(idx + 2 * PULSEW , 1) +
	     pulseamp(idx + 7 * PULSEW, 1) + pulseamp(idx + 9 * PULSEW, 1);

	/* peak detection */
	if (pv1 > pv0 && pv1 > pv2) {
		unsigned char frame[14];
		int flen = 0;
		unsigned char bits = 0;
		int k;
		float ns;
		unsigned long int ts = 0;

		lidx = idx - 1 ;

		/* noise estimation */
		ns = 2.0*(pulseamp(lidx + PULSEW, 0) + pulseamp(lidx + 8 * PULSEW , 0)); 

		/* s/n test */
		if (pv1 < ns * M_SQRT2) {
			return 1;
		}

		if (outformat)
			ts = computetimestamp(pv0, pv1, pv2, sc - 1);

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
			if (pulseamp(lidx + (16 + 2 * k) * PULSEW , 1) >
			    pulseamp(lidx + (17 + 2 * k) * PULSEW , 1))
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
		float amp,iv,qv;

		iv = (float)(iq[i]);
		qv = (float)(iq[i + 1]);
		amp = hypotf(qv,iv);

		ampbuff[inidx] = amp;
		inidx++;
		if(inidx==APBUFFSZ) {
			memcpy(ampbuff,&(ampbuff[APBUFFSZ-PULSEW-DECOFFSET]),(DECOFFSET+PULSEW)*sizeof(float));
			inidx=DECOFFSET+PULSEW;
		}	

		samplecount++;

		needsample--;
		if (needsample == 0)
			needsample = deqframe(inidx-DECOFFSET, samplecount );
	}

}
