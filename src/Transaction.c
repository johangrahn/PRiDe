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
#include <glib.h>

void Transaction_begin( Transaction *transaction, DB_ENV *databaseEnvironment, ConflictSet *conflictSet )
{
	int ret;
	pthread_mutex_t *lock;
	ret = databaseEnvironment->txn_begin( databaseEnvironment, NULL, &transaction->handler, 0 );
	
	if( ret != 0 ) {
		__ERROR( "Failed to create a new transaction" );
		databaseEnvironment->err( databaseEnvironment, ret, "Transaction begin failed!" );
	}
	
	/* Tells the conflict set that there is an transaction processing it */
	conflictSet->activeTransaction = 1;
	
	
	lock = g_hash_table_lookup( __conf.transactionLocks, conflictSet->dboid ); 
	pthread_mutex_lock( lock );
	
	__DEBUG( "Locked conflict set for a transaction" );
	
	
	/* Creates a shadow copy of the conflict set */
	transaction->conflictSet = ConflictSet_createCopy( conflictSet );
}

void Transaction_update( Transaction *transaction, MethodCallObject *methodCallObject )
{
	ConflictSet_insertLocalUpdate( transaction->conflictSet, methodCallObject );
}

void Transaction_commit( Transaction *transaction )
{
	pthread_mutex_t *lock;
	
	lock = g_hash_table_lookup( __conf.transactionLocks, transaction->conflictSet->dboid ); 
	pthread_mutex_unlock( lock );
	
	__DEBUG( "Released a transaction lock on conflict set" );
	
	g_hash_table_replace( __conf.conflictSets, transaction->conflictSet->dboid, transaction->conflictSet );
	
	ConflictSet_notifyPropagation( transaction->conflictSet );
	
	
}


