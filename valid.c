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

int errcorr = 0;
int crcok = 0;

typedef struct aircraft_s aircraft_t;
struct aircraft_s {
	uint32_t AA;
	int cnt;
	time_t t;
	aircraft_t *next;
} ;
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

static void addaircraft(uint32_t aa)
{
	aircraft_t *curr=ahead;
	aircraft_t *prev=NULL;
	int t=time(NULL);

	if(aa==0) return;

	while(curr) {
		if(curr->AA==aa) {
			if(prev) prev->next=curr->next;
			break;
		}
		if(curr->t+DT < t) {
			delaircraft(prev,curr);
			curr=NULL;
			break;;
		}
		prev=curr;
		curr=curr->next;	
	}
	if(curr==NULL) {
		curr=malloc(sizeof(aircraft_t));
		curr->AA=aa;
		curr->cnt=0;
	}
	if(curr!=ahead) {
		curr->next=ahead;
		ahead=curr;
	}
	curr->t=t;
	curr->cnt++;
}

static int findaircraft(uint32_t aa)
{
	aircraft_t *curr=ahead;
	aircraft_t *prev=NULL;
	int t=time(NULL);

	while(curr) {
		if(curr->AA==aa && curr->cnt>1 ) {
			curr->cnt++;
			return 1;
		}
		if(curr->t+DT < t) {
			delaircraft(prev,curr);
			return 1;
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

int validframe(uint8_t *frame, const int len)
{
	uint32_t crc;
	uint32_t type;
	int nb;

	type = frame[0] >> 3;
	crc = Checksum(frame, len);

	/* no error case */
	if((crc==0 && len == 14 && (type==17 || type==18)) ||
	   ((crc & 0xffff80) == 0 && len == 7 && type==11)) {
		addaircraft(icao(frame));
		return 1;
	}

	if(crcok) return 0;

	if(errcorr && len == 14 ) {
		nb = testFix(frame, crc);
		if( nb >= 0 ) {
			fixChecksum(frame,nb);
			type = frame[0] >> 3;
			if( type == 17 || type == 18 ) {
				addaircraft(icao(frame));
				return 1;
			}
			fixChecksum(frame,nb); /* undo fix */
		}
	}

	if( type == 11 || type == 17 || type == 18 ) return 0;
	if( len == 7 && type != 0 && type != 4 && type != 5) return 0;
	if(len == 14  && type != 16  && type != 20 && type != 21 && type  != 24 ) return 0;

	return findaircraft(crc);
}
