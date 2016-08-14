#ifndef _TR_NETI2P_H_
#define _TR_NETI2P_H_

#include "transmission.h"
#include <event2/event.h>
#include "net.h"

typedef struct tr_i2p_tunnel {
    _bob_ctx_t		*context;				// Bob Control Context
    _bob_tunnel_t	*tunnel;				// Bob Tunnel Context
	
    int state;							// The state of the tunnel 
    bool i2pEnabled;
	
    tr_address inbound_address;			// The address of listen interface for incoming peers
    tr_port inbound_port;					// The port of incoming peers
    int inbound_socket;					// The socket that is bound to inbound port
	
    tr_address outbound_address;			// The address to the bridge for i2p net
    tr_port outbound_port;					// The port to bridge of i2p net
	int sock_num;                        // the num of socket for connection to bob tunnel
    struct event *timer;					// Tunnel check timer...
    
} tr_i2p_tunnel ;


const tr_address *tr_netI2PGetOutboundTunnelAddress(tr_session *session);
const tr_port *tr_netI2PGetOutboundTunnelPort(tr_session *session);
const tr_port *tr_netI2PGetInboundTunnelPort(tr_session *session);

/** @brief Initialize I2P.
*    If session has I2P enabled it will firing up the tunnel.
*    @param[in] session A pointer to the active session
*    @see tr_sessionInitImpl tr_netI2PEnable
*/
void tr_netI2PInit(tr_session *session);

/** 
 *    @brief Enables the i2p tunnel.
 *    @param[in] session A pointer to the active session
 */
void tr_netI2PEnable(tr_session *session,bool enable);

/**
 * @brief Starts the tunnel using configuration from session
 * @param[in] session A pointer to the active session
 * @return BOB_OK if success.
 */
int tr_netI2PStartTunnel(tr_session *session);

/**
 * @brief Stops the tunnel.
 */
void tr_netI2PStopTunnel(tr_session *session);

/**
 * @brief Get tunnel state.
 * @return The state of the i2p tunnel
 */
int tr_netI2PTunnelState(tr_session *session);

bool tr_netI2PIsTunnelRunning(tr_session *session);

const char *tr_netI2PGetMyEndPointKey(tr_session *session);

int tr_netI2PGetInboundTunnelSocket(tr_session *session);

#endif
