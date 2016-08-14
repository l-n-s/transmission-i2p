#include "bob.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


static struct timeval _g_timeout;
static char *_g_last_error;

int _bob_connect(_bob_ctx_t *cctx) {
	
	int firsttimever=0;
	char response[512];
	// If this is the first time we are in here for this context
	// lets set the destaddr up for further use.
	if( ntohs( cctx->destination.sin_port ) != cctx->port ) {
		struct hostent *he=NULL;
		if( ( he = gethostbyname( cctx->address ) ) == NULL ) {
			_g_last_error="Failed to get  host by name from specified bob command address.";
			return BOB_FAILED;
		}
	
		cctx->socket=socket(AF_INET, SOCK_STREAM, 0);
		cctx->destination.sin_family = AF_INET;
		cctx->destination.sin_port = htons( cctx->port );
		//cctx->destination.sin_addr.s_addr = inet_addr( inet_ntoa( *(struct in_addr *)he->h_addr) );
		cctx->destination.sin_addr = *((struct in_addr *)he->h_addr);
		
		firsttimever=1;
	}
		
	if( firsttimever || cctx->socket == -1 || write( cctx->socket, "test\n", 5) != 5 ) {
		// we have been disconnected from the bob commander
		// lets reconnect
		if(cctx->socket==-1) {
			cctx->socket=socket(AF_INET, SOCK_STREAM, 0);
		}
		
		if(connect( cctx->socket, (struct sockaddr *)&cctx->destination, sizeof(struct sockaddr) ) < 0) {
			if(errno==106) 
				return BOB_OK;			// Some how we are trying to connect on already connected socket
			
			_g_last_error=strerror(errno);
			return BOB_FAILED;
		} 
		
		// Read header
		if(_bob_read_response(cctx, response,512)==BOB_FAILED) {
			_g_last_error="Failed to read BOB service header";
			return BOB_FAILED;
		}
		
		// Verify BOB service
		if(strncmp(response,"BOB ",4)!=0) {
			_g_last_error="No BOB command service verified on destination.";
			return BOB_FAILED;
		}
		
		// Read following ok
		if(_bob_read_response(cctx, response,512)==BOB_FAILED) {
			_g_last_error="Failed to read BOB service header";
			return BOB_FAILED;
		}
		
		
		
	} else {
		// Empty network buffer
		_bob_read_response(cctx, response,512);
	}
		
	
	return BOB_OK;
}

int _bob_do_command(_bob_ctx_t *cctx, _bob_tunnel_t *tunnel, char *command ) {
	char response[512]={0};
	
	
	if( _bob_connect(cctx) == BOB_OK ) {
	#ifdef _DEBUG
		fprintf(stderr,"client -> bob: %s",command);
	#endif
		if( write( cctx->socket, command, strlen(command) ) == strlen( command ) ) {
			// Lets wait some time before reading a response
			_g_timeout.tv_sec=0;
			select(0, NULL, NULL, NULL, &_g_timeout);
			
			// Handle response of command
			return _bob_handle_response( cctx, tunnel,command);
		}
		_g_last_error="Failed to send command, write failed on socket.";
		return BOB_FAILED;
	} 
	
	return BOB_FAILED;
}

int _bob_is_response_ok(char *response) {
	while(response[0]=='\n')
		response++;
	
	if(strncmp(response,"OK",2)==0) 
		return BOB_OK;
	
	//fprintf(stderr,"Failed: %s\n",response);
	_g_last_error=strdup(response+6);
	return BOB_FAILED;
}

int _bob_read_response(_bob_ctx_t *ctx, char *response,int size) {
	int bytes;
	char *respp=response;
	int res;
	fd_set readfd; 
	
	FD_ZERO(&readfd);
	FD_SET(ctx->socket,&readfd);
	
	bytes=0;
	while(1) {
		
		bytes=read(ctx->socket,respp,1);
		if(bytes<0) {
			_g_last_error="Connection to BOB service is lost while reading response.";
			return BOB_FAILED;
		} else if(bytes==1) {
			if(*respp=='\n') {
				break;
			}
			respp++;
		}
	}
	return BOB_OK;
}

// OK DATA NICKNAME: TRZ-7f000001-5495 STARTING: false RUNNING: true STOPPING: false KEYS: true QUIET: false INPORT: 51414 INHOST: 127.0.0.1 OUTPORT: 51413 OUTHOST: 127.0.0.1
int _bob_status_response(char *response,_bob_tunnel_t *tunnel) {
	while(response[0]=='\n')
		response++;
	char *starting=strstr(response,"STARTING: ");
	char *running=strstr(response,"RUNNING: ");
	char *stopping=strstr(response,"STOPPING: ");

	tunnel->status=0;
	if(starting && running && stopping) {
		if(strncmp(starting+strlen("STARTING: "),"true",4)==0)
			tunnel->status|=TUNNEL_STATUS_STARTING_FLAG;
		if(strncmp(running+strlen("RUNNING: "),"true",4)==0)
			tunnel->status|=TUNNEL_STATUS_RUNNING_FLAG;
		if(strncmp(stopping+strlen("STOPPING: "),"true",4)==0)
			tunnel->status|=TUNNEL_STATUS_STOPPING_FLAG;
		
	} 
	return BOB_OK;
}

int _bob_handle_response( _bob_ctx_t *ctx, _bob_tunnel_t *tunnel,char *command_sent) {
	char response[1024];
	memset(&response,0,512);
	
	if( _bob_read_response(ctx,response,1024) == BOB_FAILED) {
		return BOB_FAILED;
	}
	
	#ifdef _DEBUG
		fprintf(stderr,"bob -> client: %s\n",response);
	#endif
	
	if(strncmp(command_sent,"setnick ",8)==0) {
		return _bob_is_response_ok(response);
		
	} else if(strncmp(command_sent,"newkeys",7)==0) {
		// Let check if this is the key for the outbound
		if(_bob_is_response_ok(response) == BOB_OK) {
			_bob_base64_key_t empty_key;
			// lets get the key for the endpoint service
			memcpy( tunnel->key, response+3, 516);
			
			return BOB_OK;
		} else {
			return BOB_FAILED;
		}
		
	} else if(strncmp(command_sent,"setkeys ",8)==0) {
		if(_bob_is_response_ok(response)==BOB_OK) {
			// User supplied keys accepted for tunnel lets copy to tunnel context
			memcpy( tunnel->key, command_sent+8, 516);
			return BOB_OK;
		}
		return BOB_FAILED;
	} else if(strncmp(command_sent,"outhost ",8)==0) {
		return _bob_is_response_ok(response);
		
	} else if(strncmp(command_sent,"outport ",8)==0) {
		return _bob_is_response_ok(response);
		
	}  else if(strncmp(command_sent,"inhost ",7)==0) {
		return _bob_is_response_ok(response);
		
	} else if(strncmp(command_sent,"inport ",7)==0) {
		return _bob_is_response_ok(response);
		
	} else if(strncmp(command_sent,"start",5)==0) {
		return _bob_is_response_ok(response);
		
	}  else if(strncmp(command_sent,"stop",4)==0) {
		return _bob_is_response_ok(response);
		
	}    else if(strncmp(command_sent,"status",6)==0) {
		if( _bob_is_response_ok(response) ) {
			// We got a status command, lets update tunnel status
			return _bob_status_response(response,tunnel);
		} else return BOB_FAILED;
		
	}  else if(strncmp(command_sent,"clear",5)==0) {
		return _bob_is_response_ok(response);
		
	}  else if(strncmp(command_sent,"getnick ",8)==0) {
		return _bob_is_response_ok(response);
		
	} else if(strncmp(command_sent,"option ",7)==0) {
		return _bob_is_response_ok(response);
		
	} else if(strncmp(command_sent,"quit",4)==0) {
		shutdown(ctx->socket,2);
		close(ctx->socket);
		
		ctx->socket=-1;
		
		return BOB_OK;
		//return _bob_is_response_ok(response);
	}
	
}


char *bob_error() {
	return _g_last_error;
}

void bob_init() {
	#ifdef _WIN32
		WSADATA d;
		if(WSAStartup(MAKEWORD(2,0),&d)!=0)
			fprintf(stderr,"libbob: Failed to initialize winsock.");
	#endif
		
	_g_timeout.tv_sec = 0;
	_g_timeout.tv_usec = 5000;
	
}
	


_bob_ctx_t * bob_create_context(const char *address,int port) {
	_bob_ctx_t *ctx = (_bob_ctx_t *) malloc( sizeof(_bob_ctx_t) );
	memset( ctx, 0, sizeof(_bob_ctx_t));
	ctx->socket=-1;
	ctx->address = strdup(address);
	ctx->port = port;	
	return ctx;
}

void bob_free_context(_bob_ctx_t *cctx) {
	shutdown(cctx->socket,2);
	close(cctx->socket);
	free( cctx->address );
	free( cctx );
}

void  _bob_nuke_tunnel( _bob_ctx_t *cctx, _bob_tunnel_t *tunnel) {
	
	char cmd[1024];
	char *name = tunnel->name;
			   
	sprintf(cmd,"getnick %s\n",name);
	if(_bob_do_command(cctx,tunnel,cmd) == BOB_OK) {
		sprintf(cmd,"stop\n");
		_bob_do_command(cctx,tunnel,cmd);
		// Lets wait some time for tunnel to stop before clearing
		_g_timeout.tv_sec=1;
		select(0, NULL, NULL, NULL, &_g_timeout);
		
		sprintf(cmd,"clear\n");
		if(_bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
			// We waited to short.. lets hold for some secs
			_g_timeout.tv_sec=8;
			select(0, NULL, NULL, NULL, &_g_timeout);
		
			_bob_do_command(cctx,tunnel,cmd);
		}
	
}
	
}

_bob_tunnel_t* bob_create_inbound_tunnel(
		_bob_ctx_t *cctx,
		const char *name,
		const char *addr,
		 int port,
		_bob_base64_key_t *endpoint
	)
{
	char cmd[1024];
	_bob_tunnel_t *tunnel = (_bob_tunnel_t *) malloc( sizeof( _bob_tunnel_t) );
	memset( tunnel, 0, sizeof( _bob_tunnel_t) );
	tunnel->name = strdup( name );
	
	// Close existing tunnel by name if there is any
	_bob_nuke_tunnel(cctx,tunnel);

	//tunnel->name = strdup( name );
	// Setup the outbound tunnel
	sprintf(cmd,"setnick %s\n",name);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"newkeys\n");
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"inhost %s\n",addr);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"inport %d\n",port);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	return tunnel;
}

_bob_tunnel_t* bob_create_compound_tunnel(
		_bob_ctx_t *cctx,
         const char *name,
         const char *inaddr,
		 int inport,                                 
         const char *outaddr,
         int outport,                                                         
		_bob_base64_key_t endpoint
	)
{
	char cmd[1024];

       // @TODO There is a memoverriding bug here,data @ outaddr get overridden, so further get a copy of address
       char *iaddr=strdup(inaddr),*oaddr=strdup(outaddr);
	
	_bob_tunnel_t *tunnel = (_bob_tunnel_t *) malloc( sizeof( _bob_tunnel_t) );
	memset( tunnel, 0, sizeof( _bob_tunnel_t) );
	
	tunnel->name = strdup( name );

	// Close existing tunnel by name if there is any
	_bob_nuke_tunnel(cctx,tunnel);

	//tunnel->name = strdup( name );
	
	// Setup the outbound tunnel
	sprintf(cmd,"setnick %s\n",name);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	if(endpoint) {
		sprintf(cmd,"setkeys ");
		memcpy(cmd+8,endpoint,sizeof(_bob_base64_key_t));
		strcat(cmd,"\n");
		if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
			bob_free_tunnel(tunnel);
			return NULL;
		}
	} else {
		sprintf(cmd,"newkeys\n");
		if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
			bob_free_tunnel(tunnel);
			return NULL;
		}
	}
	
	sprintf(cmd,"outhost %s\n",oaddr); 
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"outport %d\n",outport);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"inhost %s\n",iaddr);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"inport %d\n",inport);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	return tunnel;
}

_bob_tunnel_t* bob_create_outbound_tunnel(
		_bob_ctx_t *cctx,
		const char *name,
		const char *addr,
		 int port,
		_bob_base64_key_t *endpoint
	)
{

	char cmd[1024];
	_bob_tunnel_t *tunnel = (_bob_tunnel_t *) malloc( sizeof( _bob_tunnel_t) );
	memset( tunnel, 0, sizeof( _bob_tunnel_t) );
	tunnel->name = strdup( name );
	
	// Close existing tunnel by name if there is any
	_bob_nuke_tunnel(cctx,tunnel);

	//tunnel->name = strdup( name );
	
	// Setup the outbound tunnel
	sprintf(cmd,"setnick %s\n",name);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"newkeys\n");
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"outhost %s\n",addr);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	sprintf(cmd,"outport %d\n",port);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_FAILED) {
		bob_free_tunnel(tunnel);
		return NULL;
	}
	
	return tunnel;
}

int bob_tunnel_option(_bob_ctx_t *cctx,_bob_tunnel_t *tunnel,const char *option,const char *value) {
	char cmd[1024];
	sprintf(cmd,"option %s=%s\n",option,value);
	return _bob_do_command(cctx,tunnel,cmd);
}

int bob_is_tunnel_running(_bob_ctx_t *cctx,_bob_tunnel_t *tunnel) {
	char cmd[1024];
	// lets check ig we can connect to bob
	if(_bob_connect(cctx)==BOB_OK) {
		
		sprintf(cmd,"getnick %s\n",tunnel->name);
		if( _bob_do_command(cctx,tunnel,cmd) == BOB_OK) {
			// We got a tunnel nickname, lets check if it's up and running
			sprintf(cmd,"status %s\n",tunnel->name);
			_bob_do_command(cctx,tunnel,cmd);
			if(tunnel->status&TUNNEL_STATUS_RUNNING_FLAG)
				return BOB_OK;
			else
				return BOB_FAILED;
		} else 
			return BOB_NO_SUCH_TUNNEL;
	} else {
		tunnel->status=BOB_CONNECT_ERROR;
	}
}

int bob_start_tunnel(_bob_ctx_t *cctx,_bob_tunnel_t *tunnel) {
	char cmd[1024];

	sprintf(cmd,"getnick %s\n",tunnel->name);
	if( _bob_do_command(cctx,tunnel,cmd) == BOB_OK) {
		sprintf(cmd,"start\n");
		return _bob_do_command(cctx,tunnel,cmd);
	}
	
	return BOB_FAILED;
	
}

int bob_stop_tunnel(_bob_ctx_t *cctx,_bob_tunnel_t *tunnel) {
	_bob_nuke_tunnel(cctx,tunnel);
}
	
void bob_free_tunnel(_bob_tunnel_t *tunnel) {
	free( tunnel->name );
	free( tunnel );
}
