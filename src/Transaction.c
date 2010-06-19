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
#include "MethodCallObject.h"
#include "Debug.h"
#include <stdlib.h>
#include <string.h>

void Transaction_create( Transaction *transaction, DB_ENV *databaseEnv )
{
	DB 	*databaseHandler;
	DB_TXN *databaseTransaction;
	DB_MPOOLFILE *poolFile;
	int ret;
	
	
	ret = db_create( &databaseHandler, databaseEnv, 0 );
	if( ret != 0 ) {
		printf( "Can't create database for transaction\n" );
		exit( 1 );
	}

	databaseTransaction = NULL;

	ret = databaseEnv->txn_begin( databaseEnv, NULL, &databaseTransaction, 0);
	if( ret != 0 ) {
		printf( "Can't start transaction\n" );
	}

	/* Removes the cache file setting */
	poolFile = databaseHandler->get_mpf( databaseHandler );
	poolFile->set_flags( poolFile, DB_MPOOL_NOFILE, 1 );
	
	ret = databaseHandler->open( databaseHandler, databaseTransaction, NULL, NULL, DB_BTREE, DB_CREATE | DB_EXCL, 0 );
	if( ret != 0 ) {
		printf( "Failed to open database\n" );
		exit( 1 );
	}

	transaction->databaseHandler = databaseHandler;
	transaction->databaseEnv = databaseEnv;
	transaction->databaseTransaction = databaseTransaction;
}

void Transaction_update( Transaction *transaction, MethodCallObject *methodCallObject )
{
	DBT key, data;
	int ret;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	key.data = &transaction->counter;
	key.size = sizeof(int);
	
	data.data = MethodCallObject_copyObject( methodCallObject );
	data.size = sizeof( MethodCallObject );	

	ret = transaction->databaseHandler->put( transaction->databaseHandler, 
											transaction->databaseTransaction,
											&key, 
											&data,
											0);
	if (ret != 0) {
		printf("Failed to store stable update on database\n");
		exit( 1 );
	}
	
	_DEBUG( transaction->logFile, "Inserted method <%s> into transaction", methodCallObject->methodName );
	transaction->counter++;

}

void Transaction_commit(Transaction *transaction, ConflictSet *conflictSet )
{
	DBC *iterator;
	DBT key, data;
	MethodCallObject *methodCallObject;
	int ret;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	
	transaction->databaseHandler->cursor( transaction->databaseHandler, 
										transaction->databaseTransaction,
										&iterator, 
										0);

	/* Iterate through all MCO:s */
	while( (ret = iterator->get( iterator, &key, &data, DB_NEXT )) == 0 ) {

		/* Make a copy of the mco so that it can be stored in conflict set */
		methodCallObject = MethodCallObject_copyObject( (MethodCallObject *)data.data );
		_DEBUG( transaction->logFile, 
				"Fetched method <%s> from transaction",
				 methodCallObject->methodName );

		ConflictSet_insertLocal( conflictSet, methodCallObject );

	}

	iterator->close( iterator );
	
	/* Commit the transaction to the environment */
	transaction->databaseTransaction->commit( transaction->databaseTransaction, 0 );	

	/* Reset pointer so it can't be used anymore */
	transaction->databaseTransaction = NULL;

	/* Close the handler to the database */
	/*
	if( transaction->databaseHandler != NULL ) {
		transaction->databaseHandler->close( transaction->databaseHandler, 0 );
	}
	*/

}
