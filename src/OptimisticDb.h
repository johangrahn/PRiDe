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

#ifndef _OPTDB_H_
#define _OPTDB_H_

#include <db.h>

#include "MethodCallObject.h"

/* The structure holds information about the database and its environment */
typedef struct {

	/* The database environment pointer */
	DB_ENV 	*envp;
	
	/* Pointer to the used database */
	DB 		*dbp;
	
	/* Set to 1 if there exists a connection to the database */
	int isOpen;

} OptimisticDb;
#endif 

/* Creates a new structure with a new environment */
OptimisticDb* optimisticDb_create();

/* Stores the MethodCallObject in the database */
void optimisticDb_store(OptimisticDb* optimisticDb, MethodCallObject *methodCallObject );

/* Opens a new database */
void optimisticDb_open( OptimisticDb *optimisticDb );
