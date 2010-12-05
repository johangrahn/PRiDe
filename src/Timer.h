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

#ifndef __TIMER_H__
#define __TIMER_H__

#include <sys/time.h>

typedef struct timeval timeval;


timeval __stable_start;
timeval __stable_end;

/* Stores the current time in the timeval structure */
void timer_mark( timeval *t );

/* Calculates the difference between two timeval:s in milliseconds */
void timer_getDiff( timeval *result,  timeval *t1, timeval *t2 );

#endif