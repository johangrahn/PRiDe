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


#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "Parameter.h"
#include "MethodCallObject.h"
#include <stdlib.h>

/* Prototype for each method in a adatabase object when resolving */
void (*prideMethodPrototype)(void *object, Parameter *params, int paramSize);

/*
 * All objects needs to have the size member at first position 
 * so that the Object Storage can store the object correctly 
 */
typedef struct {
	size_t size;
} ObjectBase;

/*
 * This structure represents a example object 
 * that is can be replicated 
 *
 * Each method has a PRiDe method that is performed
 * when the conflict resolution is done with a generation.
 * These methods have the _pride_ name scheme 
 */
typedef struct {
	size_t size;
	
	int propertyA,
		propertyB;
		
	char databaseObjectId[ PRIDE_DBOID_SIZE ];

} Object;

/* Object methods */

void Object_increaseA( Object *object, int value );
void Object_decreaseA( Object *object, int value );

void Object_increaseA_resolve( void *object, Parameter *params, int paramSize );
void Object_decreaseA_resolve( void *object, Parameter *params, int paramSize );

#endif
