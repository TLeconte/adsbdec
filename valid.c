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

static int stat_try[26] = { 0 };
static int stat_ok[26] = { 0 };
static int stat_fi[26] = { 0 };

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

int validShort(uint8_t *frame,const uint64_t ts,uint32_t crc,uint32_t pw)
{
	uint32_t type = frame[0] >> 3;

	stat_try[type]++;

	if(type == 11) {
	       	if ((crc & 0xffff80) == 0 ) {
			stat_ok[type]++;
			addaircraft(icao(frame));
			netout(frame, 7,ts,pw);
			return 1;
		} 
		return 0;
	}

	if (findaircraft(crc)) {
		stat_fi[type]++;
		netout(frame, 14,ts,pw);
		return 1;
	}

	return 0;
}

int validLong(uint8_t *frame, const uint64_t ts,uint32_t crc,uint32_t pw)
{
	uint32_t type = frame[0] >> 3;
	int nb;

	stat_try[type]++;

	if( type == 17 || type == 18 ) {
	    if(crc==0) {
		stat_ok[type]++;
		addaircraft(icao(frame));
		netout(frame, 14,ts,pw);
		return 1;
	    }

	    if(errcorr) {
		nb = testFix(frame, crc);
		if( nb >= 0 ) {
			fixChecksum(frame,nb);
			if(findaircraft(icao(frame))){
				type = frame[0] >> 3;
				stat_fi[type]++;
				netout(frame, 14,ts,pw);
				return 1;
			}
		}
	    }
	    return 0;
	}

	if (findaircraft(crc)) {
		stat_fi[type]++;
		netout(frame, 14,ts,pw);
		return 1;
	}

	return 0;
}

void print_stats(void)
{
	int tot_ok,tot_fi;

	fprintf(stderr,"\t%7d\t%7d\t%7d\t%7d\t%7d\t%7d\t%7d\t%7d\t%7d\t%7d\tTotal\n",0,4,5,11,16,17,18,20,21,24);
	fprintf(stderr,"Try :\t");
	fprintf(stderr,"%7d\t",stat_try[0]);
	fprintf(stderr,"%7d\t",stat_try[4]);
	fprintf(stderr,"%7d\t",stat_try[5]);
	fprintf(stderr,"%7d\t",stat_try[11]);
	fprintf(stderr,"%7d\t",stat_try[16]);
	fprintf(stderr,"%7d\t",stat_try[17]);
	fprintf(stderr,"%7d\t",stat_try[18]);
	fprintf(stderr,"%7d\t",stat_try[20]);
	fprintf(stderr,"%7d\t",stat_try[21]);
	fprintf(stderr,"%7d\t",stat_try[24]);
	fprintf(stderr,"%d\n",stat_try[0]+stat_try[4]+stat_try[5]+stat_try[11]+stat_try[16]+stat_try[17]+stat_try[18]+stat_try[20]+stat_try[21]+stat_try[24]);

	fprintf(stderr,"Ok :\t");
	fprintf(stderr,"%7d\t",stat_ok[0]);
	fprintf(stderr,"%7d\t",stat_ok[4]);
	fprintf(stderr,"%7d\t",stat_ok[5]);
	fprintf(stderr,"%7d\t",stat_ok[11]);
	fprintf(stderr,"%7d\t",stat_ok[16]);
	fprintf(stderr,"%7d\t",stat_ok[17]);
	fprintf(stderr,"%7d\t",stat_ok[18]);
	fprintf(stderr,"%7d\t",stat_ok[20]);
	fprintf(stderr,"%7d\t",stat_ok[21]);
	fprintf(stderr,"%7d\t",stat_ok[24]);
	tot_ok=stat_ok[0]+stat_ok[4]+stat_ok[5]+stat_ok[11]+stat_ok[16]+stat_ok[17]+stat_ok[18]+stat_ok[20]+stat_ok[21]+stat_ok[24];
	fprintf(stderr,"%d\n",tot_ok);

	fprintf(stderr,"Find :\t");
	fprintf(stderr,"%7d\t",stat_fi[0]);
	fprintf(stderr,"%7d\t",stat_fi[4]);
	fprintf(stderr,"%7d\t",stat_fi[5]);
	fprintf(stderr,"%7d\t",stat_fi[11]);
	fprintf(stderr,"%7d\t",stat_fi[16]);
	fprintf(stderr,"%7d\t",stat_fi[17]);
	fprintf(stderr,"%7d\t",stat_fi[18]);
	fprintf(stderr,"%7d\t",stat_fi[20]);
	fprintf(stderr,"%7d\t",stat_fi[21]);
	fprintf(stderr,"%7d\t",stat_fi[24]);
	tot_fi=stat_fi[0]+stat_fi[4]+stat_fi[5]+stat_fi[11]+stat_fi[16]+stat_fi[17]+stat_fi[18]+stat_fi[20]+stat_fi[21]+stat_fi[24];
	fprintf(stderr,"%d\n",tot_fi);

	fprintf(stderr,"\t\t\t\t\t\t\t\t\t\t\t%d\n",tot_ok+tot_fi);
}
