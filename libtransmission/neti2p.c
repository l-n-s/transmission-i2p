#include <assert.h>
#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/event.h> /* evtimer */
#include <string.h> /* memset */
#include <unistd.h> /* close */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "neti2p.h"
#include "transmission.h"
#include "trevent.h"
#include "platform.h"
#include "utils.h"
#include "session.h"
#include "log.h"
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include "net.h"


void tr_netI2PInit(tr_session *session) {
	session->tunnel = tr_new0(tr_i2p_tunnel, 1);
	session->tunnel->i2pEnabled = true;
	tr_netI2PEnable(session,session->isI2PEnabled);

	//start SAM too ?? Not yet

//	session->Sam3Session = tr_new0(Sam3Session, 1);	 //session type 2 = STREAM
//	sam3CreateSession(session->Sam3Session,session->I2PRouter,7656,NULL,2,"");
}

static void onNetI2PPulseTimer( int fd UNUSED, short what UNUSED, void * s) {
	tr_session *session=(tr_session*)s;
	struct timeval interval;
	interval.tv_sec=5;		// Default tunnel  check pulse time
	interval.tv_usec=1;
	
	// Probe tunnel functionality
	if( session->tunnel->context && session->tunnel->tunnel ) {
		int res=bob_is_tunnel_running(session->tunnel->context, session->tunnel->tunnel );
		switch(res) {
			case BOB_OK: {
				if(session->tunnel->state!=TUNNEL_STATE_IS_RUNNING) {
					tr_logAddDebug( _("I2P Tunnel State change '%s' to '%s'"), stateToString(session->tunnel->state), stateToString(TUNNEL_STATE_IS_RUNNING) );
					session->tunnel->state=TUNNEL_STATE_IS_RUNNING;
				}
			} break;
		
			case BOB_CONNECT_ERROR:
				if(session->tunnel->state!=TUNNEL_STATE_ROUTER_UNREACHABLE) {
					tr_logAddDebug( _("I2P Tunnel State change '%s' to '%s'"), stateToString(session->tunnel->state), stateToString(TUNNEL_STATE_ROUTER_UNREACHABLE) );
					session->tunnel->state=TUNNEL_STATE_ROUTER_UNREACHABLE;
					interval.tv_sec=10;
				}
			break;
				
			case BOB_FAILED:
				// Tunnel is there but is not running... let's try to start it..
				if( bob_start_tunnel(session->tunnel->context, session->tunnel->tunnel ) == BOB_FAILED )
					bob_start_tunnel(session->tunnel->context, session->tunnel->tunnel );
				interval.tv_sec=1;
			break;
			
			case BOB_NO_SUCH_TUNNEL:			// Bob is running but no such tunnel exists
				if(session->tunnel->state!=TUNNEL_STATE_IS_NOT_RUNNING) {
					tr_logAddDebug( _("I2P Tunnel State change '%s' to '%s'"), stateToString(session->tunnel->state), stateToString(TUNNEL_STATE_IS_NOT_RUNNING) );
					session->tunnel->state=TUNNEL_STATE_IS_NOT_RUNNING;
					// Let's start up a new tunnel and give it time to start by
					// waiting for 1 second before we pulse tunnel again.
					tr_netI2PStartTunnel(session);
					interval.tv_sec=10;
				}
			break;
		
			default:
				// Shouldn't ever get in here..
			break;
		}
	}  else {
		// No context/tunnel available lets init a tunnel if I2P enabled
		if( session->tunnel->i2pEnabled == true)  {
			tr_netI2PStartTunnel(session);
			interval.tv_sec=2;
		}
	}
	
	evtimer_add( session->tunnel->timer, &interval );
}

static void netI2PStartTimer( tr_session * session)
{
	
    session->tunnel->timer = evtimer_new(session->event_base, onNetI2PPulseTimer, session);
    onNetI2PPulseTimer( 0, 0, session );
}

static void netI2PStopTimer( tr_session *session )
{
    if( session->tunnel->timer != NULL )
    {
        evtimer_del( session->tunnel->timer );
        tr_free( session->tunnel->timer  );
        session->tunnel->timer = NULL;
    }
}

static bool netI2PInitContext(tr_session *session) {

	FILE* tmp;
	char cmd[1024] = {0};
	char buf[256]={0};
	char newname[256]={0};
	char tmpnewname[256]={0};
	char * filelock = tr_buildPath (session->configDir, ".bob-lock", NULL);
	
	if( session->tunnel->context == NULL ) {
		session->tunnel->context = bob_create_context( session->I2PRouter, session->I2PBobPort );
		if( session->tunnel->context == NULL ) 
		{
			tr_logAddDebug( _( "Failed to create context, probably cause is memory exceeded" ));
			return false;

		}

	if ((tmp = fopen(filelock,"r+")) != NULL)
	{
	while(fgets(buf, 255, tmp) != NULL)
		{
	sprintf(newname, "%s", buf);		
			
	// Close existing broken tunnel by name if there is any
	memcpy(tmpnewname,newname,strlen(newname) );
	if (tmpnewname != NULL && tmpnewname != '\0' && strlen(tmpnewname) > 0)
	  {	
		sprintf(cmd,"getnick %s\n",tmpnewname);
		if(_bob_do_command(session->tunnel->context,session->tunnel->tunnel,cmd) == BOB_OK) {
		sprintf(cmd,"stop\n");
			_bob_do_command(session->tunnel->context,session->tunnel->tunnel,cmd);
		sprintf(cmd,"clear\n");
		if(_bob_do_command(session->tunnel->context,session->tunnel->tunnel,cmd) == BOB_FAILED) {
			// We waited to short.. lets hold for some secs
			sleep(4);
		
			_bob_do_command(session->tunnel->context,session->tunnel->tunnel,cmd);
		}
			session->tunnel->context = bob_create_context( session->I2PRouter, session->I2PBobPort );
		}
		 
	  }
	  }	
	fclose(tmp);
	}	
	}
	if ((tmp = fopen(filelock,"w")) != NULL)
	{
    fputs ("", tmp);
	fclose(tmp);
    }
	tr_free (filelock);
    return true;
}

void tr_netI2PEnable(tr_session *session,bool enable) {
    
	if( session->tunnel->i2pEnabled == true && enable == false) 
	{
	    session->tunnel->i2pEnabled = false;
	    tr_logAddInfo(_("Session is disabling I2P network"));
	    netI2PStopTimer(session);
	    tr_netI2PStopTunnel(session);
	}
	else if( session->tunnel->i2pEnabled== false && enable == true) 
	{
	    session->tunnel->i2pEnabled = true;
	    tr_logAddInfo(_("Session is enabling I2P network"));
	    netI2PStartTimer(session);
	} 
	else if( session->tunnel->i2pEnabled == true && enable == true) 
	{
		//tr_netI2PStopTunnel(session);
	    tr_logAddInfo(_("Session is enabling I2P network"));
		session->tunnel->state=TUNNEL_STATE_STARTED;
	    netI2PStartTimer(session);
		// Changing to an active state
	}
}



int tr_netI2PStartTunnel(tr_session *session) {
	char name[256]={"TRZ-"};
	struct sockaddr_in addr;
	socklen_t  s = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	socklen_t size = sizeof(addr);
	uint8_t ad[PEER_ID_LEN+1];
    FILE* tmp;
	char * filelock = tr_buildPath (session->configDir, ".bob-lock", NULL);
	
    tr_logAddInfo(_("Starting up I2P tunnel for session."));
    session->tunnel->state=TUNNEL_STATE_STARTED;
    
    // Let's check if we have an i2p bob context handle if not, lets create one, otherwise fail out
    netI2PInitContext(session);

    // Let's create the inbound/outbound tunnel
    tr_logAddDebug( _("Initializing I2P / BOB Inbound/Outbound  tunnel.") );
    
    // Lets do an quick to findout wich address to bind outbound
    
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(session->I2PRouter);
	memset(&addr, 0, sizeof(struct sockaddr_in));//sizeof(addr)
    connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    memset(&addr,0,sizeof(struct sockaddr_in));//sizeof(addr)
	getsockname(s,(struct sockaddr *)&addr,&size); 
    close(s);

    //ad = (unsigned char *)&addr.sin_addr.s_addr;
	tr_peerIdInit(ad);
    // Establish the tunnel
    sprintf(name+4,"%2.2x%2.2x%2.2x%2.2x-%d", (unsigned char)*(ad+0), (unsigned char)*(ad+1),(unsigned char)*(ad+2),(unsigned char)*(ad+3), getpid());
	tr_logAddDebug( _("Setting up tunnel %s I2P outbound tunnel %s %d, inbound tunnel %s %d"),name,session->I2PRouter,session->public_peer_port-1,session->I2PRouter,session->public_peer_port-2);

    // First of all create the inbound tunnel
    /// @todo If two clients is using the same incoming base port the later client will fail creating it's tunnel.. (Maybe use a random public_peer_port ??) inet_ntop(AF_INET,&(addr.sin_addr), str, INET_ADDRSTRLEN)
    session->tunnel->tunnel = bob_create_compound_tunnel(session->tunnel->context,name,session->I2PRouter,session->public_peer_port-1,session->I2PRouter,session->public_peer_port-2,NULL);
	/// @todo How do we actually handle a failure for creating and tunnel??
    if( session->tunnel->tunnel == NULL) {
         tr_logAddDebug( _("Failed to initialize I2P / BOB tunnel with reason: %s"),bob_error());
	//	QMessageBox::critical(this, "Failed to initialize I2P / BOB tunnel with reason: %s"),bob_error());
	return BOB_FAILED;
    }
    
   	
    // Setup the tunnel
    tr_logAddDebug( _("Setting up I2P / BOB tunnel configuration based on current mode: %d\n"), session->I2PTunnelMode );

    bob_tunnel_option(session->tunnel->context,session->tunnel->tunnel,"i2cp.gzip","False");
    
    switch( session->I2PTunnelMode ) {
        case 0:		// high anonymity
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.quantity","4");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.quantity","4");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.backupQuantity","0");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.backupQuantity","0");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.length","2");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.length","2");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.lengthVariance","2");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.lengthVariance","2");
	break;
		
	case 2:		// Performance low anonymity
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.quantity","4");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.quantity","4");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.backupQuantity","0");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.backupQuantity","0");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.length","1");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.length","1");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.lengthVariance","0");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.lengthVariance","0");
	break;
		
	case 1:
	default:
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.quantity","3");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.quantity","3");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.backupQuantity","0");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.backupQuantity","0");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.length","1");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.length","1");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"outbound.lengthVariance","1");
	     bob_tunnel_option(session->tunnel->context, session->tunnel->tunnel ,"inbound.lengthVariance","1");
	break;
    }
    
    
    // Lets start the tunnel
    if( bob_start_tunnel( session->tunnel->context, session->tunnel->tunnel ) == BOB_FAILED ) {
        tr_logAddDebug( _("Failed to start I2P / BOB tunnel with reason: %s"),bob_error());
	    return BOB_FAILED;
    }
	
    // Initialize the tr_address for inbound and outbound tunnels
    session->tunnel->outbound_address.type = TR_AF_INET;
    inet_pton(AF_INET, session->I2PRouter, &session->tunnel->outbound_address.addr.addr4);
    session->tunnel->outbound_port = htons( session->public_peer_port-1);
	
    session->tunnel->inbound_address.type = TR_AF_INET;
    inet_pton(AF_INET, session->I2PRouter, &session->tunnel->inbound_address.addr.addr4);
    session->tunnel->inbound_port = htons( session->public_peer_port-2);

	                 
	if ((tmp = fopen(filelock,"a+")) != NULL)
	{
	strcat(name,"\n");
    fputs (name, tmp);
	fclose(tmp);
    }
	
    return BOB_OK;
}

void tr_netI2PStopTunnel(tr_session *session) {
	FILE* tmp;
	char * filelock = tr_buildPath (session->configDir, ".bob-lock", NULL);
	tr_logAddInfo(_("Closing down I2P tunnel\n"));
   
	netI2PStopTimer( session );
	if( session->tunnel->tunnel ) {
		bob_stop_tunnel( session->tunnel->context, session->tunnel->tunnel );
		bob_free_tunnel( session->tunnel->tunnel );
		session->tunnel->tunnel=NULL;

	if ((tmp = fopen(filelock,"w")) != NULL)
	{
    fputs ("", tmp);
	fclose(tmp);
    }
	}
	tr_free (filelock);
}

int tr_netI2PTunnelState(tr_session *session) {
	if( session->isI2PEnabled)
		return session->tunnel->state;
	else return -1;
}

bool tr_netI2PIsTunnelRunning(tr_session *session) {
	assert( tr_isSession( session ) );

	if( 
		session->isI2PEnabled &&
		tr_netI2PTunnelState(session) == TUNNEL_STATE_IS_RUNNING
	)
		return true;
	
	return false;
}

const char *tr_netI2PGetMyEndPointKey(tr_session *session) {
    assert( tr_isSession( session ) );

    if( tr_netI2PIsTunnelRunning(session) == true ) 
	return (char*)session->tunnel->tunnel->key;
    
    return NULL;
}

const
tr_address *tr_netI2PGetOutboundTunnelAddress(tr_session *session) {
	assert( tr_isSession( session ) );
	return &session->tunnel->outbound_address;
}

const
tr_port *tr_netI2PGetOutboundTunnelPort(tr_session *session) {
	assert( tr_isSession( session ) );
	return &session->tunnel->outbound_port;
}

const
tr_port *tr_netI2PGetInboundTunnelPort(tr_session *session) {
	assert( tr_isSession( session ) );
	return &session->tunnel->inbound_port;
}


/*void tr_netI2PSetInboundTunnelSocket(tr_session *session, int socket) {
	session->tunnel->inbound_socket=socket;
}*/

int tr_netI2PGetInboundTunnelSocket(tr_session *session) {
    assert( tr_isSession( session ) );
     return session->tunnel->inbound_socket;
}
