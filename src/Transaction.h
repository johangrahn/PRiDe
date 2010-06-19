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

#include <db.h>

#include "MethodCallObject.h"
#include "ConflictSet.h"

typedef struct {
	DB_TXN 	*databaseTransaction;
	DB 		*databaseHandler;
	DB_ENV 	*databaseEnv; 
	int		counter; /* Internal counter for each update */

	/* For logging */
	FILE 	*logFile;
} Transaction;

void Transaction_create( Transaction *transaction, DB_ENV *databaseEnv );

void Transaction_update( Transaction *transaction, MethodCallObject *methodCallObject );

/* Commits the transaction and add them to the conflict set */
void Transaction_commit( Transaction *transaction, ConflictSet *conflictSet );
