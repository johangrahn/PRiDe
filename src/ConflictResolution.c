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

void* conflictResolutionThread( void *data)
{
	EventQueue *completeGenerationsQueue;
	ConflictSet *conflictSet;
	
	completeGenerationsQueue = (EventQueue*) data;
	
	__DEBUG( "Starting conflict resolution thread" );
	
	while( 1 ) {
		/* Listen for new complete generation from all conflict sets */
		EventQueue_listen( completeGenerationsQueue );
		
		conflictSet = EventQueue_pop( completeGenerationsQueue );
		
		__DEBUG( "Got signal from conflict set with dboid: %s", conflictSet->dboid );
	}
	
	
	return NULL;
}

MethodCallObject* firstPolicy(Generation *generation)
{
	MethodCallObject *update;
	update = generation->generationData[0].methodCallObject;
	return update;
}
