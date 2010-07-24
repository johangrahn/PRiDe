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

#include "Transaction.h"
#include "Debug.h"
#include <string.h>

void Transaction_begin( Transaction *transaction, DB_ENV *databaseEnvironment, ConflictSet *conflictSet )
{
	int ret;
	
	ret = databaseEnvironment->txn_begin( databaseEnvironment, NULL, &transaction->handler, 0 );
	
	if( ret != 0 ) {
		__ERROR( "Failed to create a new transaction" );
		databaseEnvironment->err( databaseEnvironment, ret, "Transaction begin failed!" );
	}
	
	/* Tells the conflict set that there is an transaction processing it */
	conflictSet->activeTransaction = 1;
	
	/* Creates a shadow copy of the conflict set */
	transaction->conflictSet = ConflictSet_createCopy( conflictSet );
}

void Transaction_update( Transaction *transaction, MethodCallObject *methodCallObject )
{
	ConflictSet_insertLocalUpdate( transaction->conflictSet, methodCallObject );
}


