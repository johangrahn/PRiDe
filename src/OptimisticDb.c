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

#include "OptimisticDb.h"

#include <stdlib.h>
#include <string.h>

OptimisticDb* optimisticDb_create()
{
	OptimisticDb 	*optimisticDb;
	DB_ENV 			*envp;
	u_int32_t 		envFlags;
	int 			ret;

	
	/* Allocate memory */
	optimisticDb = malloc( sizeof( OptimisticDb ) );

	ret = db_env_create( &envp, 0 );
	if( ret != 0 ) {
		printf( "Failed to create anvironment for optimistic database store\n" );
		exit(1);
	}
	
	envFlags = 
		DB_CREATE |
		DB_INIT_LOCK |
		DB_INIT_LOG | 
		DB_INIT_TXN |
		DB_INIT_MPOOL |
		DB_PRIVATE /* Don't put region files on disk */
	;

    /* Store database logs entirely in memory */
    envp->log_set_config( envp, DB_LOG_IN_MEMORY, 1 ); 

    /* Increase the cache size  */
    envp->set_cachesize( envp, 0, 100 * 1024 * 1024, 1 ); 
    
    envp->set_errfile( envp, stderr );

	ret = envp->open( envp, NULL, envFlags, 0 );
	if( ret != 0 ) {
		printf( "Failed to create environment for optimistic database store\n");
		exit( 1 );
	}
	

	optimisticDb->envp = envp;
	optimisticDb->isOpen = 0;

	return optimisticDb;
}

void optimisticDb_store( OptimisticDb* optimisticDb, MethodCallObject *methodCallObject )
{	
	DBT				key,
					value;
	int 			ret;
	

	if( !optimisticDb->isOpen ) {
		optimisticDb_open( optimisticDb );
	}

	memset(&key, 0, sizeof( DBT ) );
	memset(&value, 0, sizeof( DBT ) );
	
	key.data = &ret;
	key.size = sizeof( int );

	value.data = methodCallObject;
	value.size = sizeof( methodCallObject );

	ret = optimisticDb->dbp->put( optimisticDb->dbp, NULL, &key, &value, 0 );
	if( ret != 0 ) {
		printf( "Failed to store method %s on optimstic storage\n", methodCallObject->methodName );
		exit( 1 );
	}

}

void optimisticDb_open( OptimisticDb *optimisticDb )
{
	DB_MPOOLFILE 	*mpf;
	DB 				*dbp;
	u_int32_t 		openFlags;
	int 			ret;

	/* Creates the database handler */
	ret = db_create( &dbp, optimisticDb->envp, 0 );
	if( ret != 0 ) {
		printf( "Can't create database for optimistic storage\n" );
		exit( 1 );
	}

	mpf = dbp->get_mpf( dbp );
	mpf->set_flags( mpf, DB_MPOOL_NOFILE, 1 );
	
	/* Opens the database file */
	openFlags = DB_CREATE | DB_EXCL;

	ret = dbp->open( dbp, NULL, NULL, NULL, DB_BTREE, openFlags, 0 );
	if( ret != 0 ) {
		printf( "Failed to open database for optimistic storage\n" );
		exit( 1 );
	}

	optimisticDb->dbp = dbp;
	optimisticDb->isOpen = 1;

}

