#include "Replica.h"

Replica* replica_find_by_id( int id )
{
	Replica *rep;
	GSList *replica_it;
	
	for( replica_it = __conf.replicas; replica_it != NULL; replica_it = g_slist_next( replica_it ) ) {
		rep = replica_it->data;
		
		if( rep->id == id ) {
			return rep;
		}
	}
	
	return NULL;
}

int replica_is_connected( Replica *rep)
{
	if( rep->socket != -1 ) {
		return 1;
	}
	else {
		return 0;
	}
}

int replica_is_done( GSList *replicas )
{
	Replica *rep;
	GSList *replica_it;
	
	for( replica_it = __conf.replicas; replica_it != NULL; replica_it = g_slist_next( replica_it ) ) {
		rep = replica_it->data;
		
		if( !replica_is_connected( rep ) ) {
			return 0;
		}
	}
	
	return 1;
}
