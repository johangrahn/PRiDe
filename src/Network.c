#include "Debug.h"
#include "Network.h"
#include "Package.h"

#include <time.h>

int networkCreateTCPSocket( char *host, int port ) 
{

   	int             connectSocket, rv;
    struct addrinfo hints,
                    *servinfo,
                    *p;
    char            port_str[10];


    memset( &hints, 0, sizeof hints );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    snprintf( port_str, sizeof(port_str), "%d", port );

    if ((rv = getaddrinfo( host, port_str, &hints, &servinfo)) != 0) {
         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
         return -1;
    }

     // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((connectSocket = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        /*
        if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        */

        if (connect( connectSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close( connectSocket );
            __DEBUG( "network_create_tcp_socket: %s", strerror(errno));
            continue;
        }

        break;
    }

    if( p == NULL )  {
        return -1;
    }

    freeaddrinfo(servinfo); // all done with this structure

    return connectSocket;

}

int networkCreateTCPServer( int port )
{

	int             listenSocket;
    struct addrinfo hints,
                    *servinfo,
                    *p;
    char            port_str[10];
    int             rv,
                    yes=1;


    memset( &hints, 0, sizeof hints );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    snprintf( port_str, sizeof(port_str), "%d", port );
    if ((rv = getaddrinfo(NULL, port_str, &hints, &servinfo)) != 0) {
         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
         exit( 1 );
    }

     // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((listenSocket = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

		
        if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
		
		
        if (bind(listenSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close( listenSocket );
            __DEBUG( "bind: %s", strerror(errno));
            continue;
        }
		
        break;
    }

    if( p == NULL )  {
        __ERROR( "Failed to bind to port: %s", port_str );
        return -1;
    }
    
    freeaddrinfo(servinfo); // all done with this structure



    if (listen(listenSocket, 10) == -1) {
        perror("listen");
        return -1;
    }


	return listenSocket;
}

void networkSetNonBlockingMode( int socket )
{
	int flags;
	
	flags = fcntl( socket, F_GETFL, 0);
	fcntl( socket, F_SETFL, flags | O_NONBLOCK );
}

int networkSendAll( int socket, void *data, int length )
{
	int totalSent;
	int bytesLeft;
	int sent;
	
	totalSent = 0;
	bytesLeft = length;
	
	/* Sends some amount of data at the time until all is sent */
	while( totalSent < length ) {
		sent = send( socket, data + totalSent, bytesLeft, 0);
		
		if( sent == -1 ) {
			__ERROR( "Failed to send %d bytes ", bytesLeft );
		}
		
		totalSent = totalSent + sent;
		bytesLeft = bytesLeft - sent;
	}
	
	return 1;
	
}
