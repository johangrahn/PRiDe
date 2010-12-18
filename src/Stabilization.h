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

#ifndef _STABILIZATION_H_
#define _STABILIZATION_H_

#include <glib.h>
#include "Generation.h"
#include "ConflictSet.h"
#include "DBoid.h"

/* Data that the stabilizator threads needs when starting up */
typedef struct {
	GHashTable *methods;
	GHashTable *objectStore;
	ConflictSet *conflictSet;
	EventQueue *stabEventQueue;
} StabilizatorThreadData;


/* Thread that is called when a generation is ready for stabilization */
void* stabilizatorThreadProcess( void *data );

/* Prototype for each method in a adatabase object when resolving */
void (*prideMethodPrototype)(void *object, Parameter *params, int paramSize);

/* 
 * Stabilizes the given generation by performing conflict resolution 
 * on the generation and then perform the choosen update on the object 
 */
void stabilize( GHashTable *objects, GHashTable *methods, Generation *generation );

int sendStabilization( GSList *replicas, int generation, int replicaId, dboid_t dbid );
	
/*
 * Sends the stbilization information to each node in the replica list 
 */ 
int sendStabilizationMessage( GSList *replicas, int startGeneration, int endGeneration, int replicaId, dboid_t dboid );

#endif
