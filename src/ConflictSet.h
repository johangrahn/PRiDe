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

#ifndef _CONFLICT_SET_H_
#define _CONFLICT_SET_H_

/* 
 * Defines the max number of generations that can be 
 * in the conflict set at the same time 
 */

#include "Generation.h"
#include "EventQueue.h"
#include "MethodCallObject.h"
//#include "ApplicationData.h"
#include <stdio.h>
#include <pthread.h>
#include <glib.h>

#ifndef PRIDE_CS_SIZE 
	#define PRIDE_CS_SIZE 500
#endif 

typedef struct _ConflictSet {

	/* Pointer to a allocated area of generations */
	Generation *generations;

	int numberOfGenerations;
	
	/* Stores the positions of the circular buffer */
	int minPosition,
		maxPosition;
	
	/* Stores the newest and oldest generation that is in the conflict set */
	int minGeneration,
		maxGeneration;	

	/* Stores the lastest generation that have been propagated */
	int propagatedGeneration;

	/* Stores the last generation that have been stabilized */
	int stabilizedGeneration;
	
	/* Stores the dboid that is used for the conflict set */
	char dboid[40];
	
	/* A mutex lock that is used for handling multiple threads accessing the conflict set */
	pthread_mutex_t writeLock;

	/* Holds a pointer to the stabilization event queue */
	EventQueue *stabEventQueue;
	EventQueue *propEventQueue;
	
	/* Is 1 when a active transaction is using hte conflict set, 0 otherwise */
	size_t activeTransaction;

} ConflictSet;

/* Sets default values to the structure */
void ConflictSet_initVars( ConflictSet *conflictSet, int numberOfGenerations );


/* Inserts a local update from the transaction to the conflict set */
void ConflictSet_insertLocalUpdate( ConflictSet *conflictSet, MethodCallObject *methodCallObject );

/* Inserts a remote update that have been recevied from a propagation package */
void ConflictSet_insertRemoteUpdate( ConflictSet *conflictSet, MethodCallObject *methodCallObject, int sourceReplicaId, int sourceGeneration );

/* Updates the conflict set with stabilization information */
void ConflictSet_updateStabilization( ConflictSet *conflictSet, int generationNumber, int replicaId );

/* Tells the conflict set to start propagate the local updates that have been performed */
void ConflictSet_notifyPropagation( ConflictSet *conflictSet );

/* Notifies the conflict set when it is time to send out stabilization messages for any created generations */
void ConflictSet_notifyStabilization( ConflictSet *conflictSet );

/* Returns 1 if the conflict is empty( no generations), 0 otherwise */
int ConflictSet_isEmpty( ConflictSet *conflictSet );

/* Checks so that there are no errors in the conflict set */
int ConflictSet_checkIntegrity( ConflictSet *conflictSet ); 

/* Returns the number of generations in the conflict set */
int ConflictSet_getSize( ConflictSet *conflictSet );

/* Returns 1 if the conflict set is full, 0 otherwise */
int ConflictSet_isFull( ConflictSet *conflictSet );

/* Creates a new copy of the conflict set */
ConflictSet* ConflictSet_createCopy( ConflictSet *conflictSet ); 

/* Creates a new generation in the conflict set */
void ConflictSet_createNewGeneration( ConflictSet *conflictSet );

/* Fetches the array position where the given generation is */
int ConflictSet_getGenerationPosition( ConflictSet *conflictSet, int generation );

/* 
 * Pops out the oldest generation in the conflict set
 * Returns NULL if there are no generations available
 */
Generation* ConflictSet_popGeneration( ConflictSet *conflictSet );

/* Prints information about the conflict set to the stream <output> */
void ConflictSet_showState( ConflictSet *conflictSet, FILE *output );

#endif

