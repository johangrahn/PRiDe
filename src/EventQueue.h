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


#ifndef __EVENT_QUEUE_H_
#define __EVENT_QUEUE_H_

#include <pthread.h>
#include <glib.h>

typedef struct _EventQueue {
	pthread_cond_t onNotEmpty;
	pthread_mutex_t notEmptyMutex,
					writeMutex;
	GQueue *queue;
} EventQueue;

/* Setups the queue and the locks and conditions */
void EventQueue_init( EventQueue *eventQueue );

/* Listens to queue for any events from the conflict sets */
void EventQueue_listen( EventQueue *eventQueue );

/* Adds a event from a conflict set */
void EventQueue_push( EventQueue *eventQueue, void *conflictSet );

/* Fetches a event in the event queue */
void* EventQueue_pop( EventQueue *eventQueue );

#endif

