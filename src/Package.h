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

#ifndef __PACKAGE_H_
#define __PACKAGE_H_

#include "MethodCallObject.h"


enum package_type {
	PACK_ID,
	PACK_PROP,
	PACK_PROP2,
	PACK_STAB,
	PACK_STAB2
};


typedef struct _Package {
	int size;
	enum package_type pack_type;	
} Package;


typedef struct _Propagation2Package {
	int size;
	enum package_type pack_type;

	int replica_id;
	char dboid[40];
	int numberOfMethodCalls;
	MethodCallObject objects[1];
} Propagation2Package;

typedef struct _Stabilization2Package {
	int size;
	enum package_type pack_type;
	
	/* Data */
	int replicaId;
	char dboid[40];
	
	int startGeneration;
	int endGeneration;
	
} Stabilization2Package;


#endif