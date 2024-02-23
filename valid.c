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
#include "aircraft.h"

extern void netout(const uint8_t *frame, const int len, const uint64_t ts,const uint32_t pw);

int errcorr = 0;

static aircraft_t *ahead=NULL;
#define DT 60

static uint32_t stat_try[26] = { 0 };
static uint32_t stat_ok[26] = { 0 };
static uint32_t stat_gs[26] = { 0 };

static void delaircraft(aircraft_t *prev,aircraft_t *curr)
{

	if(prev)
		prev->next=NULL;
	else
		ahead=NULL;

	while(curr) {
		aircraft_t *del;
		del=curr;
		curr=curr->next;	
		free(del);
	}
}

static int addaircraft(uint32_t addr)
{
	aircraft_t *curr=ahead;
	aircraft_t *prev=NULL;
	int t=time(NULL);
	int res;

	if(addr==0) return 0;

	while(curr) {
		if(curr->addr==addr) {
			if(prev) prev->next=curr->next;
			break;
		}
		if(curr->seen+DT < t) {
			delaircraft(prev,curr);
			curr=NULL;
			break;;
		}
		prev=curr;
		curr=curr->next;	
	}
	if(curr==NULL) {
		curr=malloc(sizeof(aircraft_t));
		curr->addr=addr;
		curr->messages=0;
		res=0;
	} else 
		res = 1;
	if(curr!=ahead) {
		curr->next=ahead;
		ahead=curr;
	}
	curr->seen=t;
	curr->messages++;

	return res;
}

static int findaircraft(uint32_t addr)
{
	aircraft_t *curr=ahead;
	aircraft_t *prev=NULL;
	int t=time(NULL);

	if(addr==0) return 0;

	while(curr) {
		if(curr->addr==addr && curr->messages>1 ) {
			curr->messages++;
			return 1;
		}
		if(curr->seen+DT < t) {
			delaircraft(prev,curr);
			return 0;
		}
		prev=curr;
		curr=curr->next;	
	}
	return 0;
}

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

	if(type == 11) {
	       	if ((crc & 0xffff80) == 0 ) {
			stat_ok[type]++;
			addaircraft(icao(frame));
			netout(frame,7,ts,pw);
			return 1;
		} 
		return 0;
	}

	if (findaircraft(crc)) {
		stat_gs[type]++;
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

	if( type == 17 || type == 18 ) {
	    if(crc==0) {
		stat_ok[type]++;
		addaircraft(icao(frame));
		netout(frame, 14,ts,pw);
		return 1;
	    }

	    if(errcorr) {
		int nb;
		nb = testFix(frame, crc);
		if( nb >= 0 ) {
			fixChecksum(frame,nb);
			if(findaircraft(icao(frame))){
				stat_gs[frame[0] >> 3]++;
				netout(frame, 14,ts,pw);
				return 1;
			}
		}
	    }
	    return 0;
	}

	if (findaircraft(crc)) {
		stat_gs[type]++;
		netout(frame, 14,ts,pw);
		return 1;
	}

	return 0;
}

void print_stats(void)
{
	int tot_ok,tot_fi;

	fprintf(stderr,"\t%10d\t%10d\t%10d\t%10d\t%10d\t%10d\t%10d\t%10d\t%10d\n",0,4,5,11,16,17,18,20,21);
	fprintf(stderr,"Try :\t");
	fprintf(stderr,"%10d\t",stat_try[0]);
	fprintf(stderr,"%10d\t",stat_try[4]);
	fprintf(stderr,"%10d\t",stat_try[5]);
	fprintf(stderr,"%10d\t",stat_try[11]);
	fprintf(stderr,"%10d\t",stat_try[16]);
	fprintf(stderr,"%10d\t",stat_try[17]);
	fprintf(stderr,"%10d\t",stat_try[18]);
	fprintf(stderr,"%10d\t",stat_try[20]);
	fprintf(stderr,"%10d\n",stat_try[21]);

	fprintf(stderr,"Ok :\t");
	fprintf(stderr,"%10d\t",stat_ok[0]);
	fprintf(stderr,"%10d\t",stat_ok[4]);
	fprintf(stderr,"%10d\t",stat_ok[5]);
	fprintf(stderr,"%10d\t",stat_ok[11]);
	fprintf(stderr,"%10d\t",stat_ok[16]);
	fprintf(stderr,"%10d\t",stat_ok[17]);
	fprintf(stderr,"%10d\t",stat_ok[18]);
	fprintf(stderr,"%10d\t",stat_ok[20]);
	fprintf(stderr,"%10d\n",stat_ok[21]);
	tot_ok=stat_ok[0]+stat_ok[4]+stat_ok[5]+stat_ok[11]+stat_ok[16]+stat_ok[17]+stat_ok[18]+stat_ok[20]+stat_ok[21]+stat_ok[24];

	fprintf(stderr,"Guess :\t");
	fprintf(stderr,"%10d\t",stat_gs[0]);
	fprintf(stderr,"%10d\t",stat_gs[4]);
	fprintf(stderr,"%10d\t",stat_gs[5]);
	fprintf(stderr,"%10d\t",stat_gs[11]);
	fprintf(stderr,"%10d\t",stat_gs[16]);
	fprintf(stderr,"%10d\t",stat_gs[17]);
	fprintf(stderr,"%10d\t",stat_gs[18]);
	fprintf(stderr,"%10d\t",stat_gs[20]);
	fprintf(stderr,"%10d\n",stat_gs[21]);
	tot_fi=stat_gs[0]+stat_gs[4]+stat_gs[5]+stat_gs[11]+stat_gs[16]+stat_gs[17]+stat_gs[18]+stat_gs[20]+stat_gs[21]+stat_gs[24];

	fprintf(stderr,"Total :\t%10d\n",tot_ok+tot_fi);
}
