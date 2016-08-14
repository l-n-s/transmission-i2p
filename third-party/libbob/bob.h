#ifndef _BOB_H
#define _BOB_H

#ifdef _WIN32 
	#include <windows.h>
	#include <winsock2.h>
#else
	#include <netdb.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	
#endif 

#define BOB_FAILED	0x00
#define BOB_OK	0x01
#define BOB_NO_SUCH_TUNNEL	0x02
#define BOB_CONNECT_ERROR -1

typedef unsigned char _bob_base64_key_t[517];

typedef struct {
	char *address;
	int port;
	struct sockaddr_in destination;	
	int socket;
} _bob_ctx_t;

#define TUNNEL_STATUS_STARTING_FLAG		1
#define TUNNEL_STATUS_RUNNING_FLAG			2
#define TUNNEL_STATUS_STOPPING_FLAG		4


typedef struct {
	_bob_base64_key_t key;
	unsigned char status;
	char *name;	
} _bob_tunnel_t;

/** \brief Get a error string.
	\return A string with the error message.
*/
char *bob_error(void);

/** \brief Intitialize the library. */
void bob_init(void);

/** \brief Initialize the BOB command channel context. 
	This method creates and initializes an _bob_ctx_t context, wich are passed every time
	as an handle to the bob commander, u can use as many you want, but let me remind you that
	an a tunnel created on a specified context is always a tunnel under that context.
	\param[in] address Address to the BOB command interface, usally 127.0.0.1
	\param[in] port Port on where the BOB command interface listens, 2827
	\return NULL if an error occured.
	\see bob_error
*/
_bob_ctx_t * bob_create_context(
		const char *address,
		int port
	);
	
/** \brief Initiates an compound tunnel
	An compund tunnel is an endpoint that acts as both inbound and outbound tunnel
	using the same endpoint key and name.
	
*/
_bob_tunnel_t* bob_create_compound_tunnel(
		_bob_ctx_t *cctx,
        const char *name,
        const char *inaddr,
		int inport,                                  
        const char *outaddr,
        int outport,                                  
		_bob_base64_key_t endpoint
	);

/** \brief Initiates an inbound tunnel.
	An inbound tunnel is a "TCP Network" -> "I2P endpoint" tunnel.
	\param[in] cctx A pointer to the BOB control context.
	\param[in] addr An address of the tunnel to bind, usally 127.0.0.1 
	\param[in] port An port to use for listening on for the tunnel
	\param[in] endpoint The key of the endpoint for this tunnel
	\return An pointer to a tunnel, NULL is returned and the error string is retreived
			using bob_error() function.
	\see bob_create_context
*/
_bob_tunnel_t* bob_create_inbound_tunnel(
		_bob_ctx_t *cctx,
		const char *name,
		const char *addr,
		int port,
		_bob_base64_key_t *endpoint
	);
/** \brief Initiates an outbound tunnel.
	An outbound tunnel is "I2P endpoint" -> "TCP Network" tunnel.
	\param[in] cctx A pointer to the BOB control context.
	\param[in] addr An address of the tunnel to bind, usally 127.0.0.1 
	\param[in] port An port to direct the I2P endpoint to, 80 for an webserver etc.
	\param[in] endpoint The key of the endpoint for this tunnel, if this is NULL, a new endpointkey are generated.
	\return An pointer to a tunnel, NULL is returned and the error string is retreived
			using bob_error() function.
	\see bob_create_context
*/
_bob_tunnel_t* bob_create_outbound_tunnel(
		_bob_ctx_t *cctx,
		const char *name,
		const char *addr,
		int port,
		_bob_base64_key_t *endpoint
	);

/** \brief Set tunnel options.
	\param[in] cctx A pointer to the BOB control context.
	\param[in] tunnel A pointer to the tunnel.
	\param[in] option The option to set
	\param[in] value The value to set
	\return BOB_OK if success, otherwise BOB_FAILED is returned and the error string is retreived
			using bob_error() function.
	\see bob_create_context
*/
int bob_tunnel_option(_bob_ctx_t *cctx, _bob_tunnel_t *tunnel,const char *option,const char *value);

/** \brief Start a tunnel. 
	This activates a tunnel.
	\param[in] cctx A pointer to the BOB control context.
	\param[in] tunnel A pointer to the tunnel.
	\see bob_create_context bob_create_inbound_tunnel bob_create_outbound_tunnel
*/
int bob_start_tunnel( _bob_ctx_t *cctx, _bob_tunnel_t *tunnel );

/** \brief Stop a tunnel. 
	This stops the tunnel and also removes it for further use,
	tunnel handle is invalid after this operation.
	\param[in] cctx A pointer to the BOB control context.
	\param[in] tunnel A pointer to the tunnel.
	\see bob_create_context bob_create_inbound_tunnel bob_create_outbound_tunnel
*/
int bob_stop_tunnel( _bob_ctx_t *cctx, _bob_tunnel_t *tunnel );
	
/** \brief Check if tunnel is running.
	\param[in] cctx A pointer to the BOB control context
	\param[in] tunnel A pointer to the tunnel
	\see bob_create_context
*/
int bob_is_tunnel_running(_bob_ctx_t *cctx,_bob_tunnel_t *tunnel);
	
/** \brief Frees the BOB control context.
	\param[in] cctx A pointer to the BOB control context.
		\remarks Invalidates the context handle.
*/
void bob_free_context(
		_bob_ctx_t *ctx
	);

/** \brief Frees the tunnel context.
	\param[in] tunnel A pointer to the tunnel.
	\remarks Invalidates the tunnel handle.
*/
void bob_free_tunnel(
		_bob_tunnel_t *tunnel
	);

int _bob_connect(_bob_ctx_t *cctx);

int _bob_handle_response( _bob_ctx_t *ctx, _bob_tunnel_t *tunnel,char *command_sent);

void  _bob_nuke_tunnel( _bob_ctx_t *cctx, _bob_tunnel_t *tunnel);

int _bob_do_command(_bob_ctx_t *cctx, _bob_tunnel_t *tunnel, char *command );
	
#endif
