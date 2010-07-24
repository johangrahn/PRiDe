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

#include "BDB.h"
#include "Debug.h"
#include <stdlib.h>

DB_ENV* BDB_createInMemoryEnv()
{
	DB_ENV  	*envp;
	int			ret;
	u_int32_t 	envFlags;
	
	/* Create the database enviroment */
	ret = db_env_create( &envp, 0 );
	if( ret != 0 ) {
		__ERROR( "Failed to create database enviroment in ObjectStore_init() " );
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

	/* Open database environment */
	ret = envp->open( envp, NULL, envFlags, 0 );
	if( ret != 0 ) {
		__ERROR( "Failed to open environment for database store" );
		exit( 1 );
	}
	
	return envp;
}