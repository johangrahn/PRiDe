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

#ifndef __OBJECT_STORE_H_
#define __OBJECT_STORE_H_

#include <db.h>
#include "DBoid.h"
typedef struct {
	
	/* Handler for BerkeleyDB storage */
	DB_ENV 	*environment;
	DB 		*database;
	
} ObjectStore;

/* Creates the database (in memory) and sets up the environment */
void ObjectStore_init( ObjectStore *objectStore );

/* Stores the object inside the datbase */
void ObjectStore_put( ObjectStore *objectStore, dboid_t dboid, void *object, size_t objectSize );

/* Fetches the object based on it's dboid */
void ObjectStore_fetch( ObjectStore *objectStore, dboid_t dboid, void **object, size_t objectSize );
#endif

