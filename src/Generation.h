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

#ifndef _GENERATION_H_
#define _GENERATION_H_

#include "MethodCallObject.h"

#ifndef PRIDE_NUM_REPLICAS
	#define PRIDE_NUM_REPLICAS 3
#endif
/* 
 * Represents the different types of data
 * that can be stored in a generation and on a specific 
 * replica 
 */
typedef union {
	MethodCallObject 	*methodCallObject;
	void 				*noUpdate;
	void 				*none;
} GenerationData;

/* 
 * Stores information on what type of update the given replica
 * can perform on the given generation 
 */
typedef enum {
	GEN_UPDATE,
	GEN_NO_UPDATE,
	GEN_NONE
} GenerationType;

typedef struct {
	
	GenerationType generationType[ PRIDE_NUM_REPLICAS ];
	GenerationData generationData[ PRIDE_NUM_REPLICAS ];

	int number; 

} Generation;

/* Sets the correct values to each replica in the generation */
void Generation_init( Generation *generation );

/* Clears all memory that have been allocated inside the generation */
void Generation_free( Generation *generation );

/* Clones the object */
Generation* Generation_clone( Generation *generation );

/* Checks if the generation is complete, meaning that all updates in the 
 * generation is of type UPDATE or NO_UPDATE
 */
int Generation_isComplete( Generation *generation );

#endif
