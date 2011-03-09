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

#include "Generation.h"
#include "Debug.h"
#include <stdlib.h>

void Generation_init( Generation *generation )
{
	int it;
	
	generation->number = -1;

	for(it = 0; it < PRIDE_NUM_REPLICAS; it++ ) {
		generation->generationType[ it ] = GEN_NONE;
		generation->generationData[ it ].methodCallObject = NULL;
	}
}

void Generation_free( Generation *generation )
{
	int it;

	for(it = 0; it < PRIDE_NUM_REPLICAS; it++ ) {
		switch (generation->generationType[ it ] ) {
			case GEN_UPDATE:
				free( generation->generationData[ it ].methodCallObject );
			break;

			case GEN_NO_UPDATE:
			case GEN_NONE:
			break;
		}

	}
}

Generation* Generation_clone( Generation *generation )
{
	Generation 	*generationCopy;
	int			it;

	generationCopy = malloc( sizeof( Generation ) );

	generationCopy->number = generation->number;
	for(it = 0; it < PRIDE_NUM_REPLICAS; it++ ) {

		generationCopy->generationType[ it ] = generation->generationType[ it ];

		switch (generation->generationType[ it ] ) {
			case GEN_UPDATE:
				generationCopy->generationData[it].methodCallObject = MethodCallObject_copyObject( generation->generationData[ it ].methodCallObject );
			break;

			case GEN_NO_UPDATE:
			case GEN_NONE:
			break;
		}
	}



	return generationCopy;


}

int Generation_isComplete( Generation *generation )
{
	int it;

	/* Iterate each replica and check update type 
	 * If any of the replicas have GEN_NONE, the generation is not complete
	 */
	for( it = 0; it < PRIDE_NUM_REPLICAS; it++ ) {
		if (generation->generationType[ it ] == GEN_NONE ) {
			__DEBUG( "Replica %d on generation %d is empty", it, generation->number );
			return 0;
		}
	}
	return 1;
}
