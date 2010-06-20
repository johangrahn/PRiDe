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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
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


#include "Config.h"
#include "Replica.h"
#include "Debug.h"
#include "Network.h"
#include "Package.h"
#include "Receiver.h"
#include "Propagate.h"

#include "ConflictSet.h"
#include "DBoid.h"

void pride_usage();
int pride_handle_args( int argc, char **argv );
void pride_parse_arg_str( char *string, GSList **replicas);

void pride_cleanup();
void pride_sighandler(int sig);

int main( int argc, char **argv )
{
	MethodCallObject methodCallObject;
	
	signal(SIGINT, pride_sighandler);
	signal(SIGTERM, pride_sighandler);
	
	pthread_cond_init( &__conf.listenDoneCondition, NULL );
	pthread_mutex_init(  &__conf.listenMutex, NULL );
	
	
	if( pride_handle_args( argc, argv ) == 0 ) {
		__ERROR( "Not enough arguments supplied" );
		pride_usage();
		exit( 1 );

	}
	
	
	
	ConflictSet_initVars( &__conf.conflictSet, 10 );
	
	pthread_create( &__conf.receiver, NULL, receiverThread, NULL );


	/* Waiting for the listen port to be up-and-running */
	pthread_mutex_lock( &__conf.listenMutex );
	pthread_cond_wait( &__conf.listenDoneCondition, &__conf.listenMutex );
	pthread_mutex_unlock( &__conf.listenMutex );

	/* Check if the replica is a writer */
	if( __conf.writer == 1 ) {
		strncpy( methodCallObject.databaseObjectId, "abc123", 8 );
		strncpy( methodCallObject.methodName, "method_a", 10 );
		methodCallObject.paramSize = 1;
		methodCallObject.params[0].paramType = paramTypeInt;
		methodCallObject.params[0].paramData.intData = 2;

		ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
		ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
		ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
		ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
		ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
			ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
			ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
			ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
			ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
			ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );
		ConflictSet_insertLocalUpdate( &__conf.conflictSet, &methodCallObject );	
	}
	

	while(1) {
		sleep(2);
	}

	pthread_join( __conf.receiver, NULL);
	
	return 0;
}

void pride_usage()
{
	fprintf(stdout, "\tUsage: ./pride -i <id> -l <port> -r <id:host:port> \n");
}

int pride_handle_args( int argc, char **argv )
{
	int arg;
	
	/* Setting default values */
	__conf.id = -1;
	__conf.writer = 0;
	__conf.log = stdout;
	__conf.lport = -1;

	/* Parses the arguments */
	while( (arg = getopt(argc, argv, "i:r:l:w" ) ) != -1 ) {
		
		switch( arg ) {
		
			case 'i': {
				__conf.id = atoi( optarg );

				break;
			}
			
			case 'r': {
				pride_parse_arg_str( optarg, &(__conf.replicas) );
				break;
			}	

			case 'w':
				__conf.writer = 1;
			break;
			
			case 'l':
				__conf.lport = atoi( optarg ); 
			break;
			
			default:
				return 0;
			break;
		}
	}

	if( __conf.id == -1 ) {
		return 0;
	}
	else if( __conf.lport == -1 ) {
		return 0;
	}
	else if( g_slist_length( __conf.replicas ) == 0 ) {
		return 0;	
	}

	return 1;
	
}

void pride_parse_arg_str( char *string, GSList **replicas )
{
	char *saveptr, *hostTmp;
	Replica *rep;
	
	saveptr = NULL;
	rep = malloc(sizeof( Replica ) );

	

	rep->id = atoi( strtok_r( string, ":", &saveptr ) );

	hostTmp = strtok_r( NULL, ":", &saveptr );
	strncpy( rep->host, hostTmp, sizeof( rep->host) );
	
	rep->port = atoi( strtok_r( NULL, ":", &saveptr ) );     
	
	pthread_mutex_init( &rep->replica_lock, NULL );                                   
	
	/* Reset socket value */
	rep->socket = -1;
	
//	__DEBUG( "Adding replica with id %d, host %s and port %d", rep->id, rep->host, rep->port );
	*replicas = g_slist_append(*replicas, rep);
}

void pride_cleanup()
{
	GSList *it;
	Replica *rep;

	/* Removes the socket connection from each replica */
	for( it = __conf.replicas; it != NULL; it = g_slist_next( it ) ) {
		rep = it->data;
		
		close( rep->socket );
		
		/* Removes the replica structure */
		free( rep );
	}	
	
	g_slist_free( __conf.replicas );


	/* Close all application threads */
	pthread_cancel( __conf.receiver );

	/* Closing down sockets */
	close( __conf.lsocket );
}

void pride_sighandler(int sig)
{
	__DEBUG( "Signal to close triggered, cleaning up...");
	pride_cleanup();	
	exit(1);
}




