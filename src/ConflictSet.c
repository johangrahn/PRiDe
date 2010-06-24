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

#include "ConflictSet.h"
#include "EventQueue.h"
#include "Debug.h"
#include "Propagate.h"
#include "Stabilization.h"

#include <stdlib.h>

void ConflictSet_initVars( ConflictSet *conflictSet, int numberOfGenerations )
{
	conflictSet->generations = malloc( sizeof(Generation) * numberOfGenerations );
	conflictSet->minPosition = conflictSet->maxPosition = -1;
	conflictSet->minGeneration = conflictSet->maxGeneration = -1;

	/* Setup condition variables */
	pthread_cond_init( &conflictSet->onCompleteCondition, NULL );
	pthread_mutex_init( &conflictSet->onCompleteMutex, NULL );

	conflictSet->numberOfGenerations = numberOfGenerations;
}

void ConflictSet_insertLocalUpdate( ConflictSet *conflictSet, MethodCallObject *methodCallObject)
{

	/* Check if there are any generations existing */
	if( conflictSet->maxPosition == -1 ) {

		__DEBUG( "No generation exists, creating a new one" );
		/* No generation exists, creating a new one */
		conflictSet->maxPosition = 0;
		conflictSet->minPosition = 0;
		conflictSet->minGeneration = 0;
		conflictSet->maxGeneration = 0;
	}
	else {
		if( ConflictSet_isFull( conflictSet ) ) {
			__ERROR( "Conflict set is full!" );
			//ConflictSet_showState( conflictSet, stdout );
			exit( 1 );
		}	
		else {
			conflictSet->maxPosition = (conflictSet->maxPosition + 1 ) % conflictSet->numberOfGenerations;
			conflictSet->maxGeneration ++;
		}
	}
	
	/* Add method call to the conflict set */
	Generation_init( &conflictSet->generations[conflictSet->maxPosition] );

	conflictSet->generations[conflictSet->maxPosition].generationType[0] = GEN_UPDATE;
	conflictSet->generations[conflictSet->maxPosition].generationData[0].methodCallObject = methodCallObject;
	conflictSet->generations[conflictSet->maxPosition].number = conflictSet->maxGeneration;	

	methodCallObject->generationNumber = conflictSet->maxGeneration;
	
	__DEBUG( "Added generation %d for method <%s>", conflictSet->maxGeneration, methodCallObject->methodName );


	propagate( methodCallObject, __conf.replicas );
	
	/* Notify propagator that the generation needs to be propagated */
	//EventQueue_push( conflictSet->propEventQueue, conflictSet );

	/* If the generation is complete, notify stabilizator */
	//if( Generation_isComplete( &conflictSet->generations[conflictSet->maxPosition ] ) ) {
//		EventQueue_push( conflictSet->stabEventQueue, conflictSet );
//	}
}

void ConflictSet_insertRemoteUpdate( ConflictSet *conflictSet, MethodCallObject *methodCallObject, int sourceReplicaId, int sourceGeneration )
{
	int generationPosition;
	
	generationPosition = -1;
	
	if( ConflictSet_isEmpty( conflictSet ) ) {
		__DEBUG( "No generation exists, creating a new one" );
		/* No generation exists, creating a new one */
		conflictSet->maxPosition = 0;
		conflictSet->minPosition = 0;
		conflictSet->minGeneration = 0;
		conflictSet->maxGeneration = 0;
		
		Generation_init( &conflictSet->generations[conflictSet->maxPosition] );
		conflictSet->generations[conflictSet->maxPosition].generationType[sourceReplicaId] = GEN_UPDATE;
		conflictSet->generations[conflictSet->maxPosition].generationData[sourceReplicaId].methodCallObject = methodCallObject;
		conflictSet->generations[conflictSet->maxPosition].number = conflictSet->maxGeneration;
		
		generationPosition = 0;
	}
	else { /* The conflict set is not empty */
		
		/* Check if the needed generation exists localy */
		if( sourceGeneration >= conflictSet->minGeneration && sourceGeneration <= conflictSet->maxGeneration ) {
			
			generationPosition = ConflictSet_getGenerationPosition( conflictSet, sourceGeneration );
			
			/* The update can be stored in the conflict set directly */
			conflictSet->generations[generationPosition].generationType[sourceReplicaId] = GEN_UPDATE;
			conflictSet->generations[generationPosition].generationData[sourceReplicaId].methodCallObject = methodCallObject;
			conflictSet->generations[generationPosition].number = conflictSet->maxGeneration;
		}
		else {
					
			/* Check if the generation is within the allowed span of valid generations */	
			if( sourceGeneration >= conflictSet->minGeneration && 
				( sourceGeneration - conflictSet->minGeneration ) <= conflictSet->numberOfGenerations )	{			
			
				/* Create generations until the source generation is reached */
				while( 1 ) {
				
					/* Create a new generation */
					ConflictSet_createNewGeneration( conflictSet );
			
					/* Check if this generation is the required generation for the remote update */
					if( conflictSet->maxGeneration == sourceGeneration ) {
						__DEBUG( "Create new generation(s) for remote update" );

						conflictSet->generations[conflictSet->maxPosition].generationType[sourceReplicaId] = GEN_UPDATE;
						conflictSet->generations[conflictSet->maxPosition].generationData[sourceReplicaId].methodCallObject = methodCallObject;	
						
						/* Send stabilization message to all other replicas */
						sendStabilization( __conf.replicas, conflictSet->maxGeneration, __conf.id, "Test" );
						break;			
					}
					else {
						/* Send stabilization message to all other replicas */
						sendStabilization( __conf.replicas, conflictSet->maxGeneration, __conf.id, "Test" );
					}
				
			
				}
			}
			else {
				__ERROR( "MCO <%s> with generation %d is not allowed, highest is %d", 
					methodCallObject->methodName, sourceGeneration, conflictSet->maxGeneration );
			}
			
			generationPosition = conflictSet->maxPosition;
		}
		
	}
	
	__DEBUG( "Adding remote update from replica %d on generation %d on position %d", 
		sourceReplicaId, conflictSet->maxGeneration, generationPosition );
}

void ConflictSet_updateStabilization( ConflictSet *conflictSet, int generationNumber, int replicaId )
{
	int generationPosition;
	
	generationPosition = ConflictSet_getGenerationPosition( conflictSet, generationNumber );
	if( generationPosition != -1 ) {
		conflictSet->generations[generationPosition].generationType[replicaId] = GEN_NO_UPDATE;
	
		__DEBUG( "Added stabilization information for generation %d from replica %d", generationNumber, replicaId );
	}
	else {
		__ERROR( "Failed to find generation position for generation %d in function ConflictSet_getGenerationPosition", generationNumber );
	}
}

int ConflictSet_isEmpty( ConflictSet *conflictSet )
{
	if(conflictSet->minGeneration > conflictSet->maxGeneration ||
		conflictSet->maxPosition == -1 ) {
		return 1;
	}
	else {
		return 0;
	}
}

int ConflictSet_isFull( ConflictSet *conflictSet )
{
	if( (conflictSet->maxGeneration - conflictSet->minGeneration) < ( conflictSet->numberOfGenerations - 1) ) {
		return 0;
	}
	else {
		return 1;
	}
}

int ConflictSet_getGenerationPosition( ConflictSet *conflictSet, int generation )
{
	int it;
	int generationPosition;
	
	generationPosition = conflictSet->minPosition;
	
	for( it = conflictSet->minGeneration; it <= conflictSet->maxGeneration; ++it ) {
		if( it == generation ) {
			return generationPosition;
		}
		
		/* Increase the array pointer */
		generationPosition = (generationPosition + 1) % conflictSet->numberOfGenerations;
	}
	
	return -1;
}

void ConflictSet_createNewGeneration( ConflictSet *conflictSet )
{
	/* Increase the pointer that locates the highest generation in the conflict set */
	conflictSet->maxPosition = (conflictSet->maxPosition + 1 ) % conflictSet->numberOfGenerations;
	conflictSet->maxGeneration++;
	
	Generation_init( &conflictSet->generations[conflictSet->maxPosition] );
	conflictSet->generations[conflictSet->maxPosition].number = conflictSet->maxGeneration;
}


Generation* ConflictSet_popGeneration( ConflictSet *conflictSet )
{
	Generation *generation;
	
	if( ConflictSet_isEmpty( conflictSet ) ) {
		return NULL;
	}


	/* Fetch the oldest generation */
	generation = &(conflictSet->generations[ conflictSet->minPosition ]);
	
	/* Only pop the generation if it is complete */
	if( Generation_isComplete( generation ) ) {

		/* Move the buffer one step forward */
		conflictSet->minPosition = (conflictSet->minPosition + 1) % conflictSet->numberOfGenerations;
	
		/* Increase the counter for the oldest generation number */
		conflictSet->minGeneration++;
		
		/* Creates a replica of the generation so that the data can be send to 
		 * the stabilizator 
		 */
		return Generation_clone( generation );
	
	}
	else {
		return NULL;
	}
}

void ConflictSet_showState( ConflictSet *conflictSet, FILE *output )
{	
	Generation *generation;
	int it,
		genPosition,
		replicaIt,
		maxGeneration;

	
	fprintf( output, "---\n" );
	fprintf( output, "Number of generations: %d\n", ( conflictSet->maxGeneration - conflictSet->minGeneration ) + 1  );
	fprintf( output, "Max number of generations: %d\n", conflictSet->numberOfGenerations );
	fprintf( output, "Min Generation: %d\n", conflictSet->minGeneration );	
	fprintf( output, "Max Generation: %d\n", conflictSet->maxGeneration );	
	fprintf( output, "Min Position: %d\n", conflictSet->minPosition );	
	fprintf( output, "Max Position: %d\n", conflictSet->maxPosition );	
	
	/* Check if the conflict set is empty, print only if it is not empty */
	if( !ConflictSet_isEmpty( conflictSet ) ) {

		/* Iterate each generation in the conflict set */
		genPosition = conflictSet->minPosition;
		maxGeneration = conflictSet->maxGeneration;
		for( it = conflictSet->minGeneration; it <= maxGeneration; it++ ) {

			/* Fetches the current generation */
			generation = &(conflictSet->generations[ genPosition ]);
		
			fprintf(stdout, "%d: ", generation->number );
			for( replicaIt = 0; replicaIt < PRIDE_NUM_REPLICAS; replicaIt++ ) {

				/* Detects which tyoe of update for that given replica */
				switch( generation->generationType[ replicaIt ] ) {
					
					case GEN_UPDATE:
						fprintf( output, "- [ %s ] ", generation->generationData[replicaIt].methodCallObject->methodName );
					break;

					case GEN_NO_UPDATE:
						fprintf( output, "- [ NO_UPDATE ] " );
					break;

					case GEN_NONE:
						fprintf( output, "- [ NONE ] " );
					break;

					default:
						fprintf( output, "-%d", generation->generationType[ replicaIt ] );
					break;

				}

				fprintf( output, "\n");
			}
			
			/* Increment the position in the buffer */
			genPosition = (genPosition + 1) % conflictSet->numberOfGenerations;
		}
	}
	fprintf( output, "---\n" );

	/* Flush all data to the output */
	fflush( output );
	
}
