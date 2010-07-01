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

#include "ConflictResolution.h"
#include "Debug.h"
#include "Generation.h"

void* conflictResolutionThread( void *data)
{
	EventQueue *completeGenerationsQueue;
	ConflictSet *conflictSet;
	Generation *generation;
	MethodCallObject *methodCallObject;
	
	completeGenerationsQueue = (EventQueue*) data;
	
	__DEBUG( "Starting conflict resolution thread" );
	
	while( 1 ) {
		/* Listen for new complete generation from all conflict sets */
		EventQueue_listen( completeGenerationsQueue );
		
		conflictSet = EventQueue_pop( completeGenerationsQueue );
		
		__DEBUG( "Got signal from conflict set with dboid: %s", conflictSet->dboid );
		
		/* Fetch the generation that is complete */ 
		generation = ConflictSet_popGeneration( conflictSet );
		
		if( generation != NULL) {
			__DEBUG( "Performing conflict resolution on generation %d", generation->number );
		
			methodCallObject = firstPolicy( generation );
			
			free( generation );
		}
		else {
			__DEBUG("No geneations to resolve");
		}
	}
	
	
	return NULL;
}

MethodCallObject* firstPolicy(Generation *generation)
{
	MethodCallObject 	*update;
	int 				it;
	
	/* Fetches the the update from the first replica that has an update */
	for (it = 0; it < PRIDE_NUM_REPLICAS; it++) {
		if( generation->generationType[it] == GEN_UPDATE )
			return generation->generationData[it].methodCallObject;
	}
	
	return NULL;
}
