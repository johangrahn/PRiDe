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

#ifndef __REPLICA_H_
#define __REPLICA_H_

#ifndef REPLICA_HOST_MAX
	#define REPLICA_HOST_MAX 30
#endif

#include "Config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <glib.h>

typedef struct _Replica {
	int id;
	char host[REPLICA_HOST_MAX];
	int port;
	int socket; /* Socket that is connected to the replica */
	struct addrinfo socket_info;
	pthread_mutex_t replica_lock;
} Replica;

Replica* replica_find_by_id( int id );

/* Check if the replica has a connection to this replica */
int replica_is_connected( Replica *rep);

/* Check if all the replicas have connections to this replica */
int replica_is_done( GSList *replicas );

#endif

