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

#include "DBoid.h"

#include <string.h>

dboid_t dboidCreate( char *name )
{
	uuid_t 	*uuid;
	uuid_t 	*uuid_ns;
	char 	*str;
	
	uuid_create( &uuid );
	uuid_create( &uuid_ns );
	
	uuid_load( uuid_ns, "ns:OID" );
	uuid_make( uuid, UUID_MAKE_V3, uuid_ns, name );
	
	str = NULL;
	
	uuid_export( uuid, UUID_FMT_STR, &str, NULL );
	uuid_destroy( uuid_ns );
	uuid_destroy( uuid );
	
	return str;
	
}

void dboidCopy(dboid_t dboid1, dboid_t dboid2, unsigned int size )
{
	strncpy(dboid1, dboid2, size - 1 );
	dboid1[ size - 1 ] = '\0';
}
