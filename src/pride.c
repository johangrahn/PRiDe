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
#include "ConflictResolution.h"
#include "ConflictSet.h"
#include "DBoid.h"
#include "Object.h"
#include "ObjectStore.h"
#include "BDB.h"
#include "Transaction.h"
#include "Timer.h"
#include "Watch.h"
#include <time.h>

void pride_usage();
int pride_handle_args( int argc, char **argv );
void pride_parse_arg_str( char *string, GSList **replicas);

/* function for the hash map to remove the memory allocated for a value */
void pride_delete_cs( gpointer data );


void pride_cleanup();
void pride_sighandler(int sig);
void pride_error(int sig) { __ERROR("SIGSEGV");	exit(1);}

int main( int argc, char **argv )
{
	MethodCallObject 	*methodCallObject;
	ConflictSet 		*conflictSetA, *conflictSetB;
	dboid_t 			dboidObjectA, dboidObjectB;
	EventQueue 			completeGenerationsQueue;
	Object 				objectA, objectB;
	ObjectStore 		objectStore;
	DB_ENV				*bdbEnv;
	pthread_mutex_t		transactionLockA, transactionLockB;
	Transaction 		transaction;
	int 				it;
	int 				randomNumber;
	
	signal(SIGINT, pride_sighandler );
	signal(SIGTERM, pride_sighandler );
	signal(SIGSEGV, pride_error );
	
	bdbEnv = BDB_createInMemoryEnv();
	
	ObjectStore_init( &objectStore, bdbEnv );
	__conf.objectStore = &objectStore;
	
	pthread_cond_init( &__conf.listenDoneCondition, NULL );
	pthread_mutex_init(  &__conf.listenMutex, NULL );
	
	EventQueue_init( &completeGenerationsQueue );
	
	__conf.methodList = g_hash_table_new( g_str_hash, g_str_equal );
	
	if( pride_handle_args( argc, argv ) == 0 ) {
		__ERROR( "Not enough arguments supplied" );
		pride_usage();
		exit( 1 );

	}
	
	__PRINT( "Starting up application with replica ID %d", __conf.id );
	
	dboidObjectA = dboidCreate( "object_a" );
	objectA.size = sizeof( objectA );
	objectA.propertyA = 0;
	objectA.databaseId = dboidObjectA;
	
	dboidObjectB = dboidCreate( "object_b" );
	objectB.size = sizeof( objectB );
	objectB.propertyA = 100;
	objectB.databaseId = dboidObjectB;
	
	
	
	ObjectStore_put( &objectStore, dboidObjectA, &objectA, objectA.size );
	ObjectStore_put( &objectStore, dboidObjectB, &objectB, objectB.size );

	conflictSetA = malloc( sizeof(ConflictSet) );
	conflictSetB = malloc( sizeof(ConflictSet) );
		
	ConflictSet_initVars( conflictSetA, PRIDE_CS_SIZE );
	conflictSetA->stabEventQueue = &completeGenerationsQueue;
	ConflictSet_initVars( conflictSetB, PRIDE_CS_SIZE );
	conflictSetB->stabEventQueue = &completeGenerationsQueue;
	
	dboidCopy( conflictSetA->dboid, dboidObjectA, sizeof( conflictSetA->dboid ) );
	dboidCopy( conflictSetB->dboid, dboidObjectB, sizeof( conflictSetB->dboid ) );
	
	__conf.conflictSets = g_hash_table_new_full( g_str_hash,  g_str_equal, NULL, pride_delete_cs );
	g_hash_table_insert( __conf.conflictSets, dboidObjectA, conflictSetA );
	g_hash_table_insert( __conf.conflictSets, dboidObjectB, conflictSetB );	
	/* Create the transaction locks */
	
	__conf.transactionLocks = g_hash_table_new( g_str_hash,  g_str_equal );

	pthread_mutex_init( &transactionLockA, NULL );
	pthread_mutex_init( &transactionLockB, NULL );
	g_hash_table_insert( __conf.transactionLocks, dboidObjectA, &transactionLockA );
	g_hash_table_insert( __conf.transactionLocks, dboidObjectB, &transactionLockB );
	
	/*
	 Inserts the methods that is used for the objects 
	*/
	g_hash_table_insert( __conf.methodList, "Object_increaseA", &Object_increaseA_resolve );
	
	pthread_create( &__conf.receiver, NULL, receiverThread, NULL );
	pthread_create( &__conf.conflictResolutionThreadId, NULL, conflictResolutionThread, &completeGenerationsQueue );

	/* Waiting for the listen port to be up-and-running */
	pthread_mutex_lock( &__conf.listenMutex );
	pthread_cond_wait( &__conf.listenDoneCondition, &__conf.listenMutex );
	pthread_mutex_unlock( &__conf.listenMutex );

	/* Check if the replica is a writer */
	if( __conf.writer == 1 ) {
			
		Transaction_begin( &transaction, bdbEnv, conflictSetA );
		
		srand( time( NULL ) );
		for ( it = 0; it < 30; it++ ) {
			
			randomNumber = rand() % 10;
			
			methodCallObject = malloc( sizeof( MethodCallObject ) );	
			strncpy( methodCallObject->databaseObjectId, dboidObjectA, sizeof(methodCallObject->databaseObjectId ) );
			strncpy( methodCallObject->methodName, "Object_increaseA", strlen("Object_increaseA") + 1 );
			methodCallObject->paramSize = 1;
			methodCallObject->params[0].paramType = paramTypeInt;
			methodCallObject->params[0].paramData.intData = randomNumber;
		
			Transaction_update( &transaction, methodCallObject );
		}
		
		Transaction_commit( &transaction );
		
	
		/*
		Transaction_begin( &transaction, bdbEnv, conflictSetB );
		
		methodCallObject = malloc( sizeof( MethodCallObject ) );
		strncpy( methodCallObject->databaseObjectId, dboidObjectB, sizeof(methodCallObject->databaseObjectId ) );
		strncpy( methodCallObject->methodName, "Object_increaseA", strlen("Object_increaseA") + 1 );
		methodCallObject->paramSize = 1;
		methodCallObject->params[0].paramType = paramTypeInt;
		methodCallObject->params[0].paramData.intData = 3;

		Transaction_update( &transaction, methodCallObject );
		
		methodCallObject = malloc( sizeof( MethodCallObject ) );
		strncpy( methodCallObject->databaseObjectId, dboidObjectB, sizeof(methodCallObject->databaseObjectId ) );
		strncpy( methodCallObject->methodName, "Object_increaseA", strlen("Object_increaseA") + 1 );
		methodCallObject->paramSize = 1;
		methodCallObject->params[0].paramType = paramTypeInt;
		methodCallObject->params[0].paramData.intData = 3;

		Transaction_update( &transaction, methodCallObject );
			
		methodCallObject = malloc( sizeof( MethodCallObject ) );
		strncpy( methodCallObject->databaseObjectId, dboidObjectB, sizeof(methodCallObject->databaseObjectId ) );
		strncpy( methodCallObject->methodName, "Object_increaseA", strlen("Object_increaseA") + 1 );
		methodCallObject->paramSize = 1;
		methodCallObject->params[0].paramType = paramTypeInt;
		methodCallObject->params[0].paramData.intData = 3;

		Transaction_update( &transaction, methodCallObject );		

		Transaction_commit( &transaction );
		*/
		// watch_setValue( 200 );
		// __TIME( "Starting elaped time measure" );
		// timer_mark( &__stable_start );
		
	}	
	
	

	
	while(1) {
		sleep(2);
	}

		
	pthread_join( __conf.receiver, NULL);
	pthread_join(__conf.conflictResolutionThreadId, NULL);
	return 0;
}

void pride_usage()
{
	fprintf(stdout, "\tUsage: ./pride -i <id> -l <port> -r <id:host:port> [-f logfile ] \n");
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
	while( (arg = getopt(argc, argv, "i:r:l:f:w" ) ) != -1 ) {
		
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
			
			case 'f':
				__conf.log = fopen( optarg, "w");
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
	rep->tcpSocket = -1;
	
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
		
		close( rep->tcpSocket );
		
		/* Removes the replica structure */
		free( rep );
	}	
	
	g_slist_free( __conf.replicas );


	/* Close all application threads */
	pthread_cancel( __conf.receiver );
	pthread_cancel( __conf.conflictResolutionThreadId );

	/* Closing down sockets */
	close( __conf.lsocket );
	
	/* Close logger */
	if( __conf.log != stdout )
		fclose( __conf.log );
}

void pride_sighandler(int sig)
{
	__DEBUG( "Signal to close triggered, cleaning up...");
	pride_cleanup();	
	exit(1);
}

void pride_delete_cs( gpointer data )
{
	__DEBUG( "Called delete function to delete old conflict set ");
	free( data );
}



