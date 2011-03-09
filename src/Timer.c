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

#include "Timer.h"
	
#include <stdlib.h>

void timer_mark( timeval *t)
{
	gettimeofday( t, NULL );
}

void timer_getDiff( timeval *result,  timeval *t1, timeval *t2 )
{
	int elapsed;
	
	elapsed = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
	
	result->tv_sec = elapsed / 1000000;
	result->tv_usec = elapsed % 1000000;
}
