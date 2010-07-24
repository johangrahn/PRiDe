/*
 * Copyright 2009 Johan Grahn
 *
 * This file is part of the PRiDe project 
 *
 * PRiDe is free software: you can distribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the 
 * Free Software Foundation, version 1. 
 *
 * PRiDe is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warrenty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details. 
 *
 * You should have received a copy of the GNU General Public License along with
 * PRiDe. If not, see http://www.gnu.org/licenses/
 */

#ifndef __DBOID_H_
#define __DBOID_H_

//#include <uuid.h>
	
#ifndef _UUID_T
	#ifdef __OSX 
		#include <ossp/uuid.h> 
	#else
		#include <uuid.h>
	#endif
#endif

typedef char* dboid_t;

/* Creates a new database object id from a unique name */
dboid_t dboidCreate( char *name );


void dboidCopy(dboid_t dboid1, dboid_t dboid2, unsigned int size );


#endif 
