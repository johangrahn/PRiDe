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

#ifndef _CONFLICT_RESOLUTION_H_
#define _CONFLICT_RESOLUTION_H_

#include "Generation.h"
#include "MethodCallObject.h"

#include <stdlib.h>

/* Prototype function for any conflict resolution policies */
typedef MethodCallObject* (*ConflictResolutionPolicy)( Generation *generation );

/* Simple resolution policy where the first replicas update 
 * will be used 
 */
MethodCallObject* firstPolicy( Generation *generation );

#endif 
