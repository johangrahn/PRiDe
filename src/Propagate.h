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

#ifndef __PROPAGATE_H_
#define __PROPAGATE_H_

#include "MethodCallObject.h"
#include "DBoid.h"
#include <glib.h>


/* Propagates the update to all replicas that are registered */
void propagate( MethodCallObject *methodCallObject, GSList *replicas, dboid_t dboid );

#endif
