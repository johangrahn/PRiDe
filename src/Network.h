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

#ifndef __NETWORK_H_
#define __NETWORK_H_

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#include "Replica.h"


int networkCreateTCPSocket( char *host, int port );

int networkCreateTCPServer( int port );

void networkSetNonBlockingMode( int socket );


/* 
 * Sends the data with TCP to the replicas 
 *
 * If replicas doesn't answer, wait for 25 ms, and try again
 */
void networkSendDataToAll( GSList *replicas, void *data, int dataSize );

/* Sends all data */
int networkSendAll( int socket, void *data, int length );


#endif 
