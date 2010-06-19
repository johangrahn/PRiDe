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

#ifndef _METHODCALLOBJECT_H_
#define _METHODCALLOBJECT_H_

#include "Parameter.h"

#include <stdio.h>

/* Defines the max legnth of the method name */
#ifndef MCO_MAX_METHOD_NAME 
	#define MCO_MAX_METHOD_NAME 40
#endif

/* 
 * Defines the max number of parameters that an 
 * MCO can handle
 */
#ifndef MCO_MAX_PARAMS
	#define MCO_MAX_PARAMS 5
#endif

/* Defines the size of the database object identifier */
#ifndef PRIDE_DBOID_SIZE
	#define PRIDE_DBOID_SIZE 40
#endif

/*
 * This structure holds information about a specified update
 */
typedef struct {

	char databaseObjectId[ PRIDE_DBOID_SIZE ];

	/* Stores the name of the method that have been performed */
	char methodName[ MCO_MAX_METHOD_NAME ];

	/* Sets how many parameters are used in the structure */
	int paramSize;
	
	/* Storage area for parameters */
	Parameter params[MCO_MAX_PARAMS];

	int generationNumber;
	
} MethodCallObject;

/* Copy all data into a new MethodCallObject in the heap */
MethodCallObject* MethodCallObject_copyObject( MethodCallObject *methodCallObject );

/* Displays information about the update such as method name and object id */
void MethodCallObject_showState(MethodCallObject *update, FILE *output );

#endif

