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
	
	/* Store local information about the generation */
	gen->generationType[__conf.id] = GEN_UPDATE;
	gen->generationData[__conf.id].methodCallObject = methodCallObject;	

	/* Tell the update what generation it has been stored in */
	methodCallObject->generationNumber = gen->number;
	
	__DEBUG( "Added generation %d for method <%s>", gen->number, methodCallObject->methodName );
	
	__DEBUG( "Unlocking conflict set for writing local update" );
	
	/* Unlock the structure */
	pthread_mutex_unlock( &conflictSet->writeLock );
}

void ConflictSet_insertRemoteUpdate( ConflictSet *conflictSet, 
									 MethodCallObject *methodCallObject, 
									 int sourceReplicaId, 
									 int sourceGeneration )
{
	Generation 	*gen;
	int 		generationPosition;
	int 		maxGen;
	int 		it;
	int 		failure;

	generationPosition 	= -1;
	failure 			= 0;

	/* Lock the structure */
	pthread_mutex_lock( &conflictSet->writeLock );
	
	__DEBUG( "Locking conflict set for inserting remote update" );
	
	if( ConflictSet_isEmpty( conflictSet ) ) 
	{	
		gen = ConflictSet_createNewGeneration( conflictSet );
		
		/* Insert the data into the newly created generation */
		ConflictSet_setRemoteData( conflictSet, gen, sourceReplicaId, methodCallObject );
		
		/* Set local info to NO UPDATE since the generation have been created */
		gen->generationType[__conf.id] = GEN_NO_UPDATE;
			
		__DEBUG( "ConflictSet is empty, inserting generation information into generation %d", gen->number );
		
		/* Perform conflict resolution if complete */
		ConflictSet_checkGenerationComplete( conflictSet, gen );	
	}
	else 
	{ /* The conflict set is not empty */
		
		/* Check if the needed generation exists localy */
		if( sourceGeneration >= conflictSet->minGeneration && 
			sourceGeneration <= conflictSet->maxGeneration ) 
		{	
			generationPosition = ConflictSet_getGenerationPosition( conflictSet, sourceGeneration );
			if( generationPosition == -1 ) 
			{
				__ERROR( "Failed to get generation position in ConflictSet_insertRemoteUpdate()" );
			}
			
			gen = &conflictSet->generations[generationPosition];
			
			/* Insert the data into the newly created generation */
			ConflictSet_setRemoteData( conflictSet, gen, sourceReplicaId, methodCallObject );
			
			__DEBUG( "Inserting generation information into generation %d", gen->number );
			
			/* Perform conflict resolution if complete */
			ConflictSet_checkGenerationComplete( conflictSet, gen );
		}
		else 
		{		
			/* Check if the generation is within the allowed span of valid generations */	
			if( sourceGeneration >= conflictSet->minGeneration && 
				( sourceGeneration - conflictSet->minGeneration ) <= conflictSet->numberOfGenerations )	
			{		
				
				maxGen = conflictSet->maxGeneration;
				
				/* Create the number of generations that is needed to store information about 
				 * the remote generation 
				 */
				for( it = 0; it < (sourceGeneration - maxGen); it++ )
				{
					/* Create a new generation */
					gen = ConflictSet_createNewGeneration( conflictSet );

					__DEBUG(" Creating generation since gen %d is not available", sourceGeneration );
					
					/* Set that the replica doesn't have any update on this generation */
					gen->generationType[__conf.id] = GEN_NO_UPDATE;
					
					/* Perform conflict resolution if complete */
					ConflictSet_checkGenerationComplete( conflictSet, gen );
					
				}	
				
				/* Insert the data into the newly created generation */
				ConflictSet_setRemoteData( conflictSet, gen, sourceReplicaId, methodCallObject );

				/* Set local info to NONE since the generation have been created */
				gen->generationType[__conf.id] = GEN_NO_UPDATE;
				
				__DEBUG( "Inserting generation information into generation %d", gen->number );

				/* Perform conflict resolution if complete */
				ConflictSet_checkGenerationComplete( conflictSet, gen );
			
				/* Send stabilization message for the generations that have
				 * been created 
				 */
				ConflictSet_notifyStabilization( conflictSet, gen->number );
				
			}
			else 
			{
				//__DEBUG( "MCO <%s> from replica %d with generation %d is not allowed, lowest is %d,  highest is %d", 
			//		methodCallObject->methodName, sourceReplicaId, sourceGeneration, conflictSet->minGeneration, conflictSet->maxGeneration );
				
				/* Notify about the failure */
				failure = 1;
			}
		}
	}
	
	if( failure == 0 )
	{
		__DEBUG( "Adding remote update from replica %d on generation %d", 
				sourceReplicaId, conflictSet->maxGeneration );
	}	

	ConflictSet_showState( conflictSet );

	__DEBUG( "Unlocking the conflict set for writing remote update" );
	pthread_mutex_unlock( &conflictSet->writeLock );	
}

void ConflictSet_setRemoteData( ConflictSet *conflictSet, Generation *gen, int replicaId, MethodCallObject *mco)
{
	gen->generationType[replicaId] = GEN_UPDATE;
	gen->generationData[replicaId].methodCallObject = mco;
}
	

void ConflictSet_updateStabilization( ConflictSet *conflictSet, int generationNumber, int replicaId )
{
	int generationPosition;
	int maxGeneration;
	Generation *createdGeneration;

	__DEBUG( "updateStabilization()" );

	/* Check if the generation is already stabilized */
	if( generationNumber < conflictSet->stabilizedGeneration )
	{
		return;
	}
	
	/* Check if generation is lower than the stabilized generation */
	if( generationNumber < conflictSet->minGeneration )
	{
		__DEBUG( "Failed to update stabilization, gen %d is lower than min gen %d", generationNumber, conflictSet->minGeneration );
		return;
	}

	/*
	 * Todo: Is this lock necessary if there is only one receiver process that calls this method?
	 * This is needed since conflict set needs to create new generation to store remote stabilization 
	 */
	pthread_mutex_lock( &conflictSet->writeLock );
	
	/* Checks if the generation exists in the conflict set, if not, create it */
	generationPosition = ConflictSet_getGenerationPosition( conflictSet, generationNumber );
	if( generationPosition != -1 ) 
	{
		/* Generation exists */
		conflictSet->generations[generationPosition].generationType[replicaId] = GEN_NO_UPDATE;
	
		__DEBUG( "Added stabilization information for generation %d from replica %d", generationNumber, replicaId );
		
		conflictSet->stabilizedGeneration = generationNumber;

		ConflictSet_checkGenerationComplete( conflictSet, &conflictSet->generations[generationPosition] );
	}
	else 
	{
		__DEBUG( "Generation %d needed for storing stabilization info doesn't exists, trying to create...", generationNumber );
		
		if( generationNumber < conflictSet->minGeneration && conflictSet->minGeneration != -1 ) 
		{
			//__ERROR("FFUUUUU!!!");
			//__DEBUG( "Canceled generation creation for generation %d, min is %d, max is %d  ", 
			//	generationNumber, conflictSet->minGeneration, conflictSet->maxGeneration
			//);
			pthread_mutex_unlock( &conflictSet->writeLock );
			return;
		}
		/*  
		 * Generation is not found, need to check if the generation is higher than 
		 * the maximum generation, then we need to create new generations to that given generation 
		 */
		maxGeneration = conflictSet->maxGeneration + 1;
		if( maxGeneration <= generationNumber ) 
		{	
			/* Create the number of generations that is needed to store the stabilization */
			while( maxGeneration <= generationNumber ) 
			{
				createdGeneration = ConflictSet_createNewGeneration( conflictSet );				
				createdGeneration->generationType[__conf.id] = GEN_NO_UPDATE;
				createdGeneration->generationType[replicaId] = GEN_NO_UPDATE;
				createdGeneration->number = conflictSet->maxGeneration;
				
				/* Increate the created generations counter */
				maxGeneration++;
				
				ConflictSet_checkGenerationComplete( conflictSet, &conflictSet->generations[generationPosition] );
			}
			
			__DEBUG( "Created generation %d for storing stabilization information", createdGeneration->number );
			
			conflictSet->stabilizedGeneration = generationNumber;
			ConflictSet_notifyStabilization( conflictSet, generationNumber );
		}
		else 
		{
			//__ERROR( "Stabilization message with generation %d is lower than the smalest generation number", generationNumber );
		}
	}
	
	ConflictSet_showState( conflictSet );
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
	else if( conflictSet->propagatedGeneration >= conflictSet->maxGeneration )
	{
		__DEBUG( "No need to propagate!" );
		return;
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
	
	for( currentGenPos = generationPosition; 
		currentGenPos <= conflictSet->maxPosition; 
		currentGenPos = (currentGenPos + 1 ) % conflictSet->numberOfGenerations ) 
	{	
		methodCallObject = conflictSet->generations[ currentGenPos ].generationData[__conf.id].methodCallObject;
		
		methodCalls = g_slist_append( methodCalls, methodCallObject );
		
		//propagate( methodCallObject, __conf.replicas, conflictSet->dboid );	
		conflictSet->propagatedGeneration = methodCallObject->generationNumber;
		__DEBUG( "Propagted generation %d for object with dboid %s", methodCallObject->generationNumber, methodCallObject->databaseObjectId );
	}
	
	
	/* Sends all the updates to all nodes on the network */
	propagateList( methodCalls, __conf.replicas, conflictSet->dboid );
	
	__DEBUG( "Propagated generation %d", conflictSet->propagatedGeneration );
	g_slist_free( methodCalls );
}

void ConflictSet_notifyStabilization( ConflictSet *conflictSet, int endGeneration )
{
	int startGeneration;
	
	/* Check if stabilization has been performed before */
	if( conflictSet->stabilizedGeneration == -1 ) 
	{
		startGeneration = 0;
	}
	else
	{
		/* Get the first generation that haven't been stabilized */
		startGeneration = conflictSet->stabilizedGeneration + 1;
	}

	/* Stabilize all generations that have been created
	 * This insures that the maximum number of generations are send to the replicas 
	 */
	//endGeneration = conflictSet->maxGeneration;


	/* Check that message is required */
	if( startGeneration <= endGeneration ) 
	{
		/* Sends the stabilization to all replicas */
		sendStabilizationMessage( __conf.replicas, startGeneration, endGeneration, __conf.id, conflictSet->dboid );
	
		/* Update what generations that have been send */
		conflictSet->stabilizedGeneration = endGeneration;

	}
	
	__DEBUG( "Sended stabilization from gen %d to %d", startGeneration, endGeneration );
}

void ConflictSet_checkGenerationComplete( ConflictSet *conflictSet, Generation *generation )
{
	if( Generation_isComplete( generation ) ) 
	{
		__DEBUG( "Generation %d is complete for dboid %s", generation->number, conflictSet->dboid );
		EventQueue_push( conflictSet->stabEventQueue, conflictSet->dboid );
	}

	//__DEBUG( "Generation %d is not complete", generation->number );
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
	
	// Check if generation number is out of bounds 
	if( generation < conflictSet->minGeneration || 
		generation > conflictSet->maxGeneration )
	{
		return -1;
	}

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

	/* Set default values to the new generation */
	Generation_init( &conflictSet->generations[conflictSet->maxPosition] );
	conflictSet->generations[conflictSet->maxPosition].number = conflictSet->maxGeneration;

	__DEBUG(" Created generation %d", conflictSet->maxGeneration );
	
	return &conflictSet->generations[ conflictSet->maxPosition ];
}


Generation* ConflictSet_popGeneration( ConflictSet *conflictSet )
{
	Generation *generation, *generationCopy;
	
	/* Lock the structure */
	pthread_mutex_lock( &conflictSet->writeLock );
	
	if( ConflictSet_isEmpty( conflictSet ) ) {
		
		//__DEBUG( "popGeneration(): Conflict set is empty!" );
		
		/* Unlock the structure */
		pthread_mutex_unlock( &conflictSet->writeLock );
		
		return NULL;
	}


	/* Fetch the oldest generation */
	generation = &(conflictSet->generations[ conflictSet->minPosition ]);
	
	/* Only pop the generation if it is complete */
	if( Generation_isComplete( generation ) ) 
	{

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
		__DEBUG( "popGeneration(): Generation %d is not complete", generation->number );

		/* Unlock the structure */
		pthread_mutex_unlock( &conflictSet->writeLock );
		
		return NULL;
	}
}

void ConflictSet_showState( ConflictSet *conflictSet )
{
	__DEBUG( "CS[ MAX: %d, MIN: %d, P: %d]", 
			conflictSet->maxGeneration, 
			conflictSet->minGeneration, 
			conflictSet->propagatedGeneration );
}
