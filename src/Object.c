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

#include "Object.h"
#include "Debug.h"

void Object_increaseA( Object *object, int value )
{
	__DEBUG(" Increased value by %d", value );
	object->propertyA += value;
	
	__DEBUG( "Value of object's property A with dboid <%s> is now: %d\n", object->databaseId, object->propertyA );
}

void Object_decreaseA( Object *object, int value )
{
	object->propertyA -= value;
}


void Object_increaseA_resolve( void  *object, Parameter *params, int paramSize )
{
	Object_increaseA(object, params[0].paramData.intData );
}

void Object_decreaseA_resolve( void  *object, Parameter *params, int paramSize )
{
	Object_decreaseA(object, params[0].paramData.intData );
}
