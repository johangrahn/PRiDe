#include "Propagate.h"
#include "Package.h"
#include "Config.h"
#include "Debug.h"
#include "Network.h"
#include "DBoid.h"

void propagate( MethodCallObject *methodCallObject, GSList *replicas, dboid_t dboid )
{
	PropagationPackage ppack;
	
	/* Create the propagation package */
	ppack.size = sizeof( PropagationPackage );
	ppack.pack_type = PACK_PROP;
	ppack.replica_id = __conf.id;
	dboidCopy( ppack.dboid, dboid, sizeof( ppack.dboid ) );
	
	ppack.methodCallObject = *methodCallObject;
	ppack.generationNumber = methodCallObject->generationNumber;
	
	__DEBUG( "propagating generation %d ", methodCallObject->generationNumber );
	networkSendDataToAll( replicas, &ppack, ppack.size );
}
