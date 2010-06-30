#include "EventQueue.h"
#include "ConflictSet.h"
#include "Debug.h"

void EventQueue_init( EventQueue *eventQueue )
{
	eventQueue->queue = g_queue_new();

	pthread_cond_init( &eventQueue->onNotEmpty, NULL );
	pthread_mutex_init( &eventQueue->notEmptyMutex, NULL );
	pthread_mutex_init( &eventQueue->writeMutex, NULL );
}

void EventQueue_listen( EventQueue *eventQueue )
{
	pthread_mutex_lock( &eventQueue->notEmptyMutex );	
	if( g_queue_is_empty( eventQueue->queue ) ) {
		pthread_cond_wait( &eventQueue->onNotEmpty, &eventQueue->notEmptyMutex );
	}
	pthread_mutex_unlock( &eventQueue->notEmptyMutex );	
}

void EventQueue_push( EventQueue *eventQueue, void *conflictSet )
{
	pthread_mutex_lock( &eventQueue->writeMutex );
	g_queue_push_tail( eventQueue->queue, conflictSet );
	pthread_mutex_unlock( &eventQueue->writeMutex );
	
	pthread_mutex_lock( &eventQueue->notEmptyMutex );	
	pthread_cond_signal( &eventQueue->onNotEmpty );
	pthread_mutex_unlock( &eventQueue->notEmptyMutex );	
	
}

void* EventQueue_pop( EventQueue *eventQueue )
{
	ConflictSet *conflictSet;
	
	pthread_mutex_lock( &eventQueue->writeMutex );
	conflictSet = g_queue_pop_head( eventQueue->queue );
	pthread_mutex_unlock( &eventQueue->writeMutex );

	return conflictSet;
}


