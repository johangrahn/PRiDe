#include "Propagate.h"
#include "Package.h"
#include "Config.h"
#include "Debug.h"
#include "Network.h"

void propagate( MethodCallObject *methodCallObject, GSList *replicas )
{
	GSList *it;
	int rep_socket;
	Replica *rep;
	PropagationPackage ppack;
	
	/* Create the propagation package */
	ppack.size = sizeof( PropagationPackage );
	ppack.pack_type = PACK_PROP;
	ppack.replica_id = __conf.id;
	
	ppack.methodCallObject = *methodCallObject;
	ppack.generationNumber = methodCallObject->generationNumber;
	
	/* 
	 * Create a connection to the replica 
	 * Send the package to the replica
	 * Finally, close the connection 
	 */ 
	for (it = replicas; it != NULL; it = g_slist_next( it ) ) {
		rep = it->data;
		
		/* Try to make a connection, if failure, wait 250ms */
		while(1) {
			rep_socket = networkCreateTCPSocket( rep->host, rep->port );
			if( rep_socket == -1 ) {
				__DEBUG( "Failed to connect to host %s on port %d", rep->host, rep->port );
				usleep( 250000 );
			}
			else {
				break;
			}
		}
		
		//__DEBUG( "Sending %lud bytes", sizeof( struct prop_package ) );
		
		if( networkSendAll( rep_socket, &ppack, sizeof( PropagationPackage ) ) == -1 ) {
			__ERROR( "Failed to send propagation data: %s", strerror( errno ) );
		}
		
		/* We are done with this replica, close connection */
		close( rep_socket );
		
	}
}
