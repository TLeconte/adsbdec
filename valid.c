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

extern void netout(const uint8_t *frame, const int len, const uint64_t ts);

int errcorr = 0;
int crcok = 0;

static aircraft_t *ahead=NULL;
#define DT 60

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

int validShort(uint8_t *frame,const uint64_t ts,uint32_t crc)
{
	uint32_t type = frame[0] >> 3;

	if( (crc & 0xffff80) == 0 && type==11 ) {
		if (addaircraft(icao(frame))) {
			netout(frame, 7,ts);
		}
		return 1;
	}
	return 0;
}

int validLong(uint8_t *frame, const uint64_t ts,uint32_t crc)
{
	uint32_t type = frame[0] >> 3;
	int nb;

	/* no error DF17/18 case */
	if(crc==0 && (type==17 || type==18)) {
		if(addaircraft(icao(frame))){
			netout(frame, 14,ts);
		}
		return 1;
	}

	if(errcorr) {
		nb = testFix(frame, crc);
		if( nb >= 0 ) {
			fixChecksum(frame,nb);
			type = frame[0] >> 3;
			if( type == 17 || type == 18 ) {
				if(addaircraft(icao(frame))){
					netout(frame, 14,ts);
				}
				return 1;
			}
			fixChecksum(frame,nb); /* undo fix */
		}
	}

	if( type == 17 || type == 18 ) return 0;

	if(crcok) return 0;

	if( type != 16  && type != 20 && type != 21 && type  != 24 ) return 0;

	if (findaircraft(crc)) {
		netout(frame, 14,ts);
		return 1;
	}

	return 0;
}
