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

#include "ObjectStore.h"
#include "Object.h"
#include "Debug.h"

#include <stdlib.h>
#include <string.h>

/* Opens/creates a database in the main memory */
void ObjectStore_openDatabase( ObjectStore *objectStore );

void ObjectStore_init( ObjectStore *objectStore,  DB_ENV *env )
{	
	objectStore->environment = env;
	
	ObjectStore_openDatabase( objectStore );
}

void ObjectStore_openDatabase( ObjectStore *objectStore ) 
{
	DB_MPOOLFILE 	*mpf;
	DB 				*dbp;
	u_int32_t 		openFlags;
	int 			ret;

	/* Creates the database handler */
	ret = db_create( &dbp, objectStore->environment, 0 );
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
	
	objectStore->database = dbp;

}

void ObjectStore_put( ObjectStore *objectStore, dboid_t dboid, void *object, size_t objectSize )
{
	DBT		key,
			value;
	int 	ret;
	
	memset(&key, 0, sizeof( DBT ) );
	memset(&value, 0, sizeof( DBT ) );
	
	key.data = dboid;
	key.size = strlen(dboid) + 1;
	
	value.data = object;
	value.size = objectSize;
	
	ret = objectStore->database->put( objectStore->database, NULL, &key, &value, 0 );
	if( ret != 0 ) {
		__ERROR( "Failed to store object with dboid %s", dboid );
		exit( 1 );
	}
	
	//__DEBUG( "Stored object with dboid %s", dboid );
}


void ObjectStore_fetch( ObjectStore *objectStore, dboid_t dboid, void **object, size_t objectSize )
{
	
	DBT			key,
				value;
	int 		ret;	
	ObjectBase 	*base;
	
	base = (ObjectBase*) object;
	
	memset(&key, 0, sizeof( DBT ) );
	memset(&value, 0, sizeof( DBT ) );
	
	key.data = dboid;
	key.size = strlen(dboid) + 1;
	
	//value.ulen 	= base->size;
	value.flags = DB_DBT_MALLOC;
	
	ret = objectStore->database->get( objectStore->database, NULL, &key, &value, 0 );
	if( ret != 0 ) {
		__ERROR( "Failed to fetch object with dboid %s", dboid );
		exit( 1 );
	}
	
	*object = value.data;
	
	//__DEBUG( "Fetched object with dboid %s", dboid );
}
