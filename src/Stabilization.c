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

#include "Stabilization.h"
#include "ConflictResolution.h"
#include "MethodCallObject.h"
#include "EventQueue.h"

#include "Debug.h"

#include <string.h>
#include <unistd.h>

void* stabilizatorThreadProcess( void *data )
{
	ConflictSet *conflictSet;
	StabilizatorThreadData *threadData;
	Generation *generation;

	threadData = (StabilizatorThreadData*) data;

	/* Wait for the other threads to sync up */
	pthread_barrier_wait( threadData->syncBarrier ); 
	
	while( 1 ) {

		/* Wait for conflict set to notify when there is any complete
		 * generation
		 */
		EventQueue_listen( threadData->stabEventQueue );	
		
		/* Fetch the conflict set that sended the event */
		conflictSet = EventQueue_pop( threadData->stabEventQueue );
		
		_DEBUG( conflictSet->logFile, "Got signal from conflict set\n" );

		
		/* Fetch each generation that is complete and stabilize it */
		while( (generation = ConflictSet_popGeneration( conflictSet )) != NULL) {
			stabilize( threadData->objectStore, threadData->methods, generation );
			_DEBUG( conflictSet->logFile, "Stabilized generation %d\n", generation->number );

			/* Remove old generation information */
			Generation_clear( generation );
			free( generation );

		}
		
	}
}

void stabilize( GHashTable *objectStore, GHashTable *methods, Generation *generation )
{
	MethodCallObject 			*update;
	void 						*object;
	ConflictResolutionPolicy 	policy;

	/* 
	 * Sets the conflict resolution policy that are used when 
	 * decideing which update that are going to be used 
	 */
	policy = &firstPolicy;

	/* Resolve what update to apply */
	update = policy( generation );
	/*
	fprintf( stdout, "Applying method: " );
	MethodCallObject_showState( update, stdout );
	fprintf( stdout, " to stable db\n");
	fflush( stdout );
	*/
	/* Fetch the object method function pointer */
	prideMethodPrototype = g_hash_table_lookup( methods, update->methodName );
	
	/* Fetch the object that is going to receive the update */
	object = g_hash_table_lookup( objectStore, update->databaseObjectId );

	/* Apply the update on the given object */
	prideMethodPrototype( object, update->params, update->paramSize );
	
}

