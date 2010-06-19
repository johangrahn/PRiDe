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

#include "MethodCallObject.h"
#include "Debug.h"
#include <stdlib.h>
#include <string.h>

MethodCallObject* MethodCallObject_copyObject( MethodCallObject *methodCallObject )
{
	MethodCallObject *methodCallObjectCopy;
	int it;

	methodCallObjectCopy = malloc( sizeof(MethodCallObject) );
	if( methodCallObjectCopy == NULL ) {
		__ERROR( "Failed to alloc memory" );
	}

	strncpy( methodCallObjectCopy->databaseObjectId, 
			methodCallObject->databaseObjectId, 
			PRIDE_DBOID_SIZE );

	strncpy( methodCallObjectCopy->methodName, 
			methodCallObject->methodName, 
			MCO_MAX_METHOD_NAME );
	
	methodCallObjectCopy->paramSize = methodCallObject->paramSize;

	for( it = 0; it < methodCallObject->paramSize; ++it ) {
		methodCallObjectCopy->params[it] = methodCallObject->params[it];
	}
	
	return methodCallObjectCopy;
}

void MethodCallObject_showState(MethodCallObject *update, FILE *output )
{
	fprintf( output, "<%s %s %d>", update->methodName, update->databaseObjectId, update->paramSize );
}
