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

#include "Receiver.h"
#include "Debug.h"
#include "Network.h"
#include "Package.h"
#include "ConflictSet.h"

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

/* Unpack the packed data from the buffer into packages  */
void recevierHandleData( char *dataBuffer, int dataSize, ConflictSet *conflictSet );

void* receiverThread(void *data)
{	
	char buffer[2048];
	int socket;
	int remote_socket;
	
	socklen_t addr_len;
	int numbytes;
	struct sockaddr_storage their_addr;
	fd_set *master, read_fds;
	int fdmax;
	int i;

	master = &__conf.master;

	addr_len = sizeof( their_addr );
	

	__DEBUG( "Create listen socket" );
		
	/* Creates the socket that the replica will listen to */
	socket = networkCreateTCPServer( __conf.lport );

	/* Adds the socket to the select structure */
	FD_SET( socket, master );
	fdmax = socket;

	/* Tell the main app that the listen port is up */
	pthread_mutex_lock( &__conf.listenMutex );
	pthread_cond_signal( &__conf.listenDoneCondition );
	pthread_mutex_unlock( &__conf.listenMutex );

	__DEBUG( "Waiting for connections..." );
		while(1) {

			read_fds = *master;

			if( select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1 ) {
				__ERROR( "select: %s", strerror(errno) );
			}

			/* Check for data to read */
			for( i = 0; i <= fdmax; i++ ) {
				if(FD_ISSET( i, &read_fds )) {

					/* Found new connection */
					if( i == socket ) {
						remote_socket = accept( socket, (struct sockaddr *)&their_addr, &addr_len );

						if( remote_socket == -1 ) {
							__ERROR( "accept " );
						}
						else {
							
							
							FD_SET( remote_socket, master );
							if( remote_socket > fdmax ) {
								fdmax = remote_socket;
							}
							
							
							networkSetNonBlockingMode( remote_socket );
						}
						
					}
					else {					
						__DEBUG( "Incoming data" );
						numbytes = recv(i, buffer, sizeof( buffer ), 0 );
						if(numbytes > 0 ) {
							
							recevierHandleData( buffer, numbytes, &__conf.conflictSet );
													
						}
						else if( numbytes == 0) {
							__DEBUG( "No more data, closing socket...");
							close( i );
							FD_CLR( i, master );
						}	
						else {
							__ERROR( "recv: %s", strerror(errno) );
						}
					}
				}

			}		
		}
		
		pthread_exit( NULL );
	
}

void recevierHandleData( char *dataBuffer, int dataSize, ConflictSet *conflictSet )
{
	char 					*bufferPointer;		/* Pointer to the current data */
	int 					dataLeft; 			/* How much bytes there are left to handle */
	int						dataPackageSize;
	Package					*dataPackage;
	PropagationPackage 		*propagationPackage;
	StabilizationPackage	*stabilizationPackage;
	
	/* Set the pointer to the start */
	bufferPointer = dataBuffer;
	
	dataLeft = dataSize;
	
	/* Parse the data */
	while( dataLeft > 0 ) {
		
		if( dataLeft < sizeof( Package ) ) {
			__ERROR( "Data package to smal, aborting" );
		}
		
		/* Convert data into a standard package */
		dataPackage = (Package*) bufferPointer;
		dataPackageSize = dataPackage->size;
		
		/* Detect what type of package it is */
		switch( dataPackage->pack_type ) {
			
			case PACK_PROP: 
				propagationPackage = (PropagationPackage*) bufferPointer;
				__DEBUG( "Got Propagation package from replica %d with %d bytes", propagationPackage->replica_id, dataPackageSize );
				__DEBUG( "Database OID: %s, method name: %s", 
					propagationPackage->methodCallObject.databaseObjectId, 
					propagationPackage->methodCallObject.methodName );
					
				ConflictSet_insertRemoteUpdate( conflictSet, 
					&propagationPackage->methodCallObject, 
					propagationPackage->replica_id, 
					propagationPackage->generationNumber );
			break;
			
			case PACK_STAB: 
				stabilizationPackage = (StabilizationPackage *) bufferPointer;
				__DEBUG( "Got stabilization package from replica %d with %d bytes", stabilizationPackage->replica_id, dataPackageSize );
			break;
			
			default:
				__DEBUG( "Unknown package type" );
			break;
		}
		
		/* Move data pointer to the next package */
		bufferPointer = bufferPointer + dataPackageSize;
		
		/* Counting down how much data is left to analyze */
		dataLeft = dataLeft - dataPackageSize;
		
		__DEBUG( "Remaining bytes: %d", dataLeft );
		
	}
	
	
	
}