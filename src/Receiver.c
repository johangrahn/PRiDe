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
void recevierHandleData( char *dataBuffer, int dataSize );

/* Fetches the data from the socket into the buffer array */
int receiverGetPackage( int socket, char *buffer );

void* receiverThread(void *data)
{
	char buffer[2048];
	int socket;
	int remote_socket;
	
	socklen_t addr_len;
	int numBytes;
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
						/* Fetch package from the data stream that is 
						 * ready in the socket 
						 */ 
						numBytes = receiverGetPackage( i, buffer );					
						if(numBytes > 0 ) 
						{	
							recevierHandleData( buffer, numBytes );							
						}
						else if( numBytes == 0) 
						{
							__DEBUG( "No more data, closing socket...");
							close( i );
							FD_CLR( i, master );
						}	
						else 
						{
							__ERROR( "recv: %s", strerror(errno) );
						}
					}
				}

			}		
		}
		
		pthread_exit( NULL );
	
}

void recevierHandleData( char *dataBuffer, int dataSize )
{	
	char 					*bufferPointer;		/* Pointer to the current data */
	int 					dataLeft; 			/* How much bytes there are left to handle */
	int						dataPackageSize;
	int						it;
	int						lastGeneration;
	Package					*dataPackage;
	Propagation2Package		*propPackage;
	Stabilization2Package 	*stabPackage;
	ConflictSet				*conflictSet;
	pthread_mutex_t			*transactionLock;
	
		
	if( dataSize < sizeof( Package ) ) {
		__ERROR( "Data package to smal, aborting" );
	}
	
	/* Convert data into a standard package */
	dataPackage = (Package*) dataBuffer;
	dataPackageSize = dataPackage->size;
	
	/* Detect what type of package it is */
	switch( dataPackage->pack_type ) {
		
		case PACK_PROP2:
			propPackage = (Propagation2Package *) dataBuffer;
			
			__DEBUG( "Got propagation 2 package from replica %d with dboid %s with range {%d,%d}", 
					propPackage->replica_id, 
					propPackage->dboid, 
					propPackage->objects[0].generationNumber,
					propPackage->objects[ propPackage->numberOfMethodCalls -1 ].generationNumber );

			__DEBUG( "Propagation package contains %d generations ", propPackage->numberOfMethodCalls );
			
			/* Wait for any transaction to complete first */
			transactionLock = g_hash_table_lookup( __conf.transactionLocks, propPackage->dboid );
			pthread_mutex_lock( transactionLock );
			
			/* Get the conflict set for the update */
			conflictSet = g_hash_table_lookup( __conf.conflictSets, propPackage->dboid );
			
			/* Inserts all updates */ 	
			for( it = 0; it < propPackage->numberOfMethodCalls; it++ ) 
			{
				__DEBUG( "Iserting method %s", propPackage->objects[it].methodName );
				/* Stores the updte inside the conflict set 
				 * Note that the code create a copy of the method call object so that 
				 * there is now overrite when new packages arraive 
				 */  
				ConflictSet_insertRemoteUpdate( conflictSet, 
					MethodCallObject_copyObject( &propPackage->objects[it] ), 
					propPackage->replica_id, 
					propPackage->objects[it].generationNumber );

				lastGeneration = propPackage->objects[it].generationNumber;
			}
			

			__DEBUG( "Need to stabilize to generation %d", lastGeneration);
			/* Notifies the conflict that it is time to send stabilization messages */	
			ConflictSet_notifyStabilization( conflictSet, lastGeneration );
			
			
			
			/* Unlock the conflict set */
			pthread_mutex_unlock( transactionLock );
		break;
			
		/* Handler for the improved stabilization package */
		case PACK_STAB2:
			stabPackage = (Stabilization2Package *) dataBuffer;
			__DEBUG( "Got stabilization 2 package from replica %d with dboid %s with gen range {%d, %d}", 
			stabPackage->replicaId, stabPackage->dboid, stabPackage->startGeneration, stabPackage->endGeneration  );
						
			/* Wait for any transaction to complete first */
			transactionLock = g_hash_table_lookup( __conf.transactionLocks, stabPackage->dboid );
			pthread_mutex_lock( transactionLock );
			
			/* Get the conflict set for the update */
			conflictSet = g_hash_table_lookup( __conf.conflictSets, stabPackage->dboid );
			
			/* Inserts all stabilization messages into the conflict set */
			for( it = stabPackage->startGeneration; it <= stabPackage->endGeneration; it++ )
			{
				ConflictSet_updateStabilization( conflictSet, it, stabPackage->replicaId);
			}
			
			/* Unlock the conflict set */
			pthread_mutex_unlock( transactionLock );
			
		break;
		
		default:
			__WARNING( "Unknown package type!!!!!" );
		break;
	}
}

int receiverGetPackage( int socket, char *buffer )
{
	int packageLength;
	int bytes;
	int received;
	
	/* Reads the package length from the stream */
	if( recv( socket, &packageLength, sizeof(packageLength), MSG_PEEK ) == -1 ) 
	{
		__ERROR( "Failed to read length of package" );
		return -1;
	}
	
	__DEBUG(" Found package with size %d", packageLength );
	
	/* Fetch the package from the stream */
	received = recv( socket, buffer, packageLength, 0 );
	if( received == -1 ) 
	{
		__ERROR( "Failed to read package from stream");
		return -1;
	}
	
	/* Not all data is fetched, need to fetch again */ 
	if( received < packageLength ) 
	{
		__DEBUG( "Not all data is received, trying again" );
		while( received < packageLength )
		{
			bytes = recv( socket, buffer + received, packageLength, 0 );
			if( bytes == -1 ) 
			{
				__ERROR( "Failed to read package from stream");
				return -1;
			}
			
			received += bytes;
		}
	}
	
	return packageLength;
}
