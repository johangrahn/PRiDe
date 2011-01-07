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
#include <string.h>

void ConflictSet_initVars( ConflictSet *conflictSet, int numberOfGenerations )
{
	conflictSet->generations = malloc( sizeof(Generation) * numberOfGenerations );
	conflictSet->minPosition = conflictSet->maxPosition = -1;
	conflictSet->minGeneration = conflictSet->maxGeneration = -1;
	conflictSet->propagatedGeneration = -1;
	conflictSet->stabilizedGeneration = -1;
	
	/* Setup mutex */
	pthread_mutex_init( &conflictSet->writeLock, NULL );

	conflictSet->numberOfGenerations = numberOfGenerations;
	
	conflictSet->activeTransaction = 0;
	
}

void ConflictSet_insertLocalUpdate( ConflictSet *conflictSet, MethodCallObject *methodCallObject)
{
	Generation *gen;
	
	/* Lock the structures */
	pthread_mutex_lock( &conflictSet->writeLock );
	__DEBUG( "Locking conflict set for local update" );
	
	if( ConflictSet_isFull( conflictSet ) ) 
	{
		__ERROR( "Conflict set is full!" );
		exit( 1 );
	}	
	
	gen = ConflictSet_createNewGeneration( conflictSet );
	
	gen->generationType[__conf.id] = GEN_UPDATE;
	gen->generationData[__conf.id].methodCallObject = methodCallObject;
	gen->number = conflictSet->maxGeneration;	

	methodCallObject->generationNumber = conflictSet->maxGeneration;
	
	__DEBUG( "Added generation %d for method <%s>", conflictSet->maxGeneration, methodCallObject->methodName );
	
	__DEBUG( "Unlocking conflict set for writing local update" );
	
	/* Unlock the structure */
	pthread_mutex_unlock( &conflictSet->writeLock );
}

void ConflictSet_insertRemoteUpdate( ConflictSet *conflictSet, MethodCallObject *methodCallObject, int sourceReplicaId, int sourceGeneration )
{
	Generation *gen;
	int generationPosition;
	
	generationPosition = -1;
	
	/* Lock the structure */
	pthread_mutex_lock( &conflictSet->writeLock );
	
	__DEBUG( "Locking conflict set for inserting remote update" );
	
	if( ConflictSet_isEmpty( conflictSet ) ) 
	{	
		gen = ConflictSet_createNewGeneration( conflictSet );
		
		/* Insert the data into the newly created generation */
		ConflictSet_setRemoteData( conflictSet, gen, sourceReplicaId, methodCallObject );
		
		/* Perform conflict resolution if complete */
		ConflictSet_checkGenerationComplete( conflictSet, gen );	
	}
	else 
	{ /* The conflict set is not empty */
		
		/* Check if the needed generation exists localy */
		if( sourceGeneration >= conflictSet->minGeneration && sourceGeneration <= conflictSet->maxGeneration ) 
		{	
			generationPosition = ConflictSet_getGenerationPosition( conflictSet, sourceGeneration );
			if( generationPosition == -1 ) 
			{
				__ERROR( "Failed to get generation position in ConflictSet_insertRemoteUpdate()" );
			}
			
			gen = &conflictSet->generations[generationPosition];
			
			/* Insert the data into the newly created generation */
			ConflictSet_setRemoteData( conflictSet, gen, sourceReplicaId, methodCallObject );
	
			/* Perform conflict resolution if complete */
			ConflictSet_checkGenerationComplete( conflictSet, gen );
		}
		else 
		{		
			/* Check if the generation is within the allowed span of valid generations */	
			if( sourceGeneration >= conflictSet->minGeneration && 
				( sourceGeneration - conflictSet->minGeneration ) <= conflictSet->numberOfGenerations )	
			{			
				/* Create generations until the source generation is reached */
				while( 1 ) 
				{
					/* Create a new generation */
					gen = ConflictSet_createNewGeneration( conflictSet );
			
					/* Check if this generation is the required generation for the remote update */
					if( conflictSet->maxGeneration == sourceGeneration ) 
					{
						/* Insert the data into the newly created generation */
						ConflictSet_setRemoteData( conflictSet, gen, sourceReplicaId, methodCallObject );

						/* Perform conflict resolution if complete */
						ConflictSet_checkGenerationComplete( conflictSet, gen );
						
						/* Target generation has been reached, no need to continue */
						break;			
					}
					else 
					{
						/* Set that the replica doesn't have any update on this generation */
						gen->generationType[sourceReplicaId] = GEN_NO_UPDATE;
						gen->number = conflictSet->maxGeneration;
						
						/* Perform conflict resolution if complete */
						ConflictSet_checkGenerationComplete( conflictSet, gen );
					}
				}
			}
			else 
			{
				__ERROR( "MCO <%s> with generation %d is not allowed, highest is %d", 
					methodCallObject->methodName, sourceGeneration, conflictSet->maxGeneration );
			}
		}
	}
	
	__DEBUG( "Adding remote update from replica %d on generation %d", 
		sourceReplicaId, conflictSet->maxGeneration );
		
	__DEBUG( "Unlocking the conflict set for writing remote update" );
	pthread_mutex_unlock( &conflictSet->writeLock );	
}

void ConflictSet_setRemoteData( ConflictSet *conflictSet, Generation *gen, int replicaId, MethodCallObject *mco)
{
	gen->generationType[replicaId] = GEN_UPDATE;
	gen->generationData[replicaId].methodCallObject = mco;
	gen->number = conflictSet->maxGeneration;
		
	/* Set that the replica doesn't have any update on this generation */
	gen->generationType[__conf.id] = GEN_NO_UPDATE;
}
	

void ConflictSet_updateStabilization( ConflictSet *conflictSet, int generationNumber, int replicaId )
{
	int generationPosition;
	int maxGeneration;
	Generation *createdGeneration;
	
	/*
	 * Todo: Is this lock necessary if there is only one receiver process that calls this method?
	 * This is needed since conflict set needs to create new generation to store remote stabilization 
	 */
	pthread_mutex_lock( &conflictSet->writeLock );
	
	generationPosition = ConflictSet_getGenerationPosition( conflictSet, generationNumber );
	if( generationPosition != -1 ) {
		conflictSet->generations[generationPosition].generationType[replicaId] = GEN_NO_UPDATE;
	
		__DEBUG( "Added stabilization information for generation %d from replica %d", generationNumber, replicaId );
		
		if( Generation_isComplete( &conflictSet->generations[generationPosition] ) ) {
			__DEBUG( "Generation %d is complete for dboid %s", generationNumber, conflictSet->dboid );
			EventQueue_push( conflictSet->stabEventQueue, conflictSet->dboid );
		}
	}
	else {
		__DEBUG( "Trying to insert stabilization in generation %d into generation that doesn't exists, max is %d ", generationNumber, conflictSet->maxGeneration );
		__DEBUG( "Inserting stabilization message into generation %d that needs to be created", generationNumber );
		
		/*  
		 * Generation is not found, need to check if the generation is higher than 
		 * the maximum generation, then we need to create new generations to that given generation 
		 */
		maxGeneration = conflictSet->maxGeneration + 1;
		if( maxGeneration <= generationNumber ) {
			
			/* Create the number of generations that is needed to store the stabilization */
			while( maxGeneration <= generationNumber ) {
				ConflictSet_createNewGeneration( conflictSet );
				createdGeneration = &(conflictSet->generations[conflictSet->maxPosition]);
				
				createdGeneration->generationType[__conf.id] = GEN_NO_UPDATE;
				createdGeneration->generationType[replicaId] = GEN_NO_UPDATE;
				createdGeneration->number = conflictSet->maxGeneration;
				
				__DEBUG( "Created generation %d for storing stabilization information", createdGeneration->number );
				
				/* Increate the created generations counter */
				maxGeneration++;
				
				/*__ERROR( "Created generation %d for stabilization", maxGeneration );*/
				
				/* Need to check if the generation that was created also is completed */
				if( Generation_isComplete( createdGeneration ) ) {
					__DEBUG( "Generation %d is complete for dboid %s", maxGeneration, conflictSet->dboid );
					EventQueue_push( conflictSet->stabEventQueue, conflictSet->dboid );
				}
			}
			
			
			ConflictSet_notifyStabilization( conflictSet );
			
		}
		else {
			__ERROR( "Stabilization message with generation %d is lower than the smalest generation number", generationNumber );
		}
	}
	
	pthread_mutex_unlock( &conflictSet->writeLock );
}

void ConflictSet_notifyPropagation( ConflictSet *conflictSet )
{
	int 				generationPosition, currentGenPos;
	MethodCallObject 	*methodCallObject;
	GSList 				*methodCalls;
	
	methodCalls = NULL;
	/* Check if no prior proagation has been performed */
	if( conflictSet->propagatedGeneration == -1 ) {
		generationPosition = 0;
	}
	else 
	{
		/* Get the first generation that is not propagated */
		generationPosition = ConflictSet_getGenerationPosition( conflictSet, conflictSet->propagatedGeneration + 1 );
		if( generationPosition == -1 )
		{
			__ERROR( "Failed to get generation position in notifyPropagation()" );
		} 
	}
	
	__DEBUG( "Starting to propagate from generation set with index: %d", generationPosition );
	
	for (currentGenPos = generationPosition; currentGenPos <= conflictSet->maxPosition; currentGenPos = (currentGenPos + 1 ) % conflictSet->numberOfGenerations ) 
	{	
		methodCallObject = conflictSet->generations[ currentGenPos ].generationData[__conf.id].methodCallObject;
		
		methodCalls = g_slist_append( methodCalls, methodCallObject );
		
		//propagate( methodCallObject, __conf.replicas, conflictSet->dboid );	
		conflictSet->propagatedGeneration = methodCallObject->generationNumber;
		//__DEBUG( "Propagted generation %d for object with dboid %s", methodCallObject->generationNumber, methodCallObject->databaseObjectId );
	}
	
	
	/* Sends all the updates to all nodes on the network */
	propagateList( methodCalls, __conf.replicas, conflictSet->dboid );
	
	g_slist_free( methodCalls );
}

void ConflictSet_notifyStabilization( ConflictSet *conflictSet )
{
	int startGeneration;
	int endGeneration;
	
	/* Check if stabilization has been performed before */
	if( conflictSet->stabilizedGeneration == -1) 
	{
		startGeneration = 0;
		endGeneration = conflictSet->maxGeneration;
	}
	else
	{
		/* Get the first generation that haven't been stabilized */
		startGeneration = conflictSet->stabilizedGeneration + 1;
		
		/* Stabilize all generations that have been created
		 * This insures that the maximum number of generations are send to the replicas 
		 */
		endGeneration = conflictSet->maxGeneration;
	}

	/* Check that message is required */
	if( startGeneration <= endGeneration ) 
	{
		/* Sends the stabilization to all replicas */
		sendStabilizationMessage( __conf.replicas, startGeneration, endGeneration, __conf.id, conflictSet->dboid );
	
		/* Update what generations that have been send */
		conflictSet->stabilizedGeneration = endGeneration;
	}
}

void ConflictSet_checkGenerationComplete( ConflictSet *conflictSet, Generation *generation )
{
	if( Generation_isComplete( generation ) ) 
	{
		__DEBUG( "Generation %d is complete for dboid %s", generation->number, conflictSet->dboid );
		EventQueue_push( conflictSet->stabEventQueue, conflictSet->dboid );
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

int ConflictSet_getSize( ConflictSet *conflictSet )
{
	if( ConflictSet_isEmpty( conflictSet ) ) {
		return 0;
	}
	
	return conflictSet->maxGeneration - conflictSet->minGeneration;
}


int ConflictSet_isFull( ConflictSet *conflictSet )
{
	/* 
	 * Check if the number of generations that have been inserted is less than the number of maximum generations
	 */
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
	
	__DEBUG("Failed in getGenerationPosition: min=%d,max=%d,gen=%d", conflictSet->minGeneration, conflictSet->maxGeneration, generation );
	
	return -1;
}

ConflictSet* ConflictSet_createCopy( ConflictSet *conflictSet )
{
	ConflictSet *new;
	
	/* Allocate new memory for the shadow copy */
	new = malloc( sizeof( ConflictSet ) );
	
	memcpy( new, conflictSet, sizeof( ConflictSet ) );
	
	return new;
}


Generation* ConflictSet_createNewGeneration( ConflictSet *conflictSet )
{
	/* Check if there are any generations existing */
	if( conflictSet->maxPosition == -1 ) 
	{	
		/* No generation exists, creating a new one */
		conflictSet->maxPosition = 0;
		conflictSet->minPosition = 0;
		conflictSet->minGeneration = 0;
		conflictSet->maxGeneration = 0;
	}
	else 
	{
		/* Increase the pointer that locates the highest generation in the conflict set */
		conflictSet->maxPosition = (conflictSet->maxPosition + 1 ) % conflictSet->numberOfGenerations;
		conflictSet->maxGeneration++;
	}
	
	Generation_init( &conflictSet->generations[conflictSet->maxPosition] );
	conflictSet->generations[conflictSet->maxPosition].number = conflictSet->maxGeneration;
	
	return &conflictSet->generations[conflictSet->maxPosition];
}


Generation* ConflictSet_popGeneration( ConflictSet *conflictSet )
{
	Generation *generation, *generationCopy;
	
	/* Lock the structure */
	pthread_mutex_lock( &conflictSet->writeLock );
	
	if( ConflictSet_isEmpty( conflictSet ) ) {
		
		__ERROR( "popGeneration(): Conflict set is empty!" );
		/* Unlock the structure */
		pthread_mutex_unlock( &conflictSet->writeLock );
		
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
		
		/* Creates a copy of the generation so that the data can be send to 
		 * the stabilizator 
		 */
		generationCopy = Generation_clone( generation );
		
		/* Removes the generation data from the memory */
		Generation_free( generation );
		
		/* Unlock the structure */
		pthread_mutex_unlock( &conflictSet->writeLock );
		
		return generationCopy;
	
	}
	else {
		__ERROR( "popGeneration(): Generation %d is not complete", generation->number );

		/* Unlock the structure */
		pthread_mutex_unlock( &conflictSet->writeLock );
		
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
	fprintf( output, "DBoid: %s\n", conflictSet->dboid );
	
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
