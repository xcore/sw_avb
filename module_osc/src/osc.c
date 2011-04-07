#include "osc_types.h"
#include "xccompat.h"
//#include "xtcp_client.h"
#include <string.h>


void osc_init(chanend tcp_svr, unsigned int osc_port)
{

}

int get_osc_version(char a0[]) 
{
  strcpy(a0,"1.1");
  return 1;
}

int get_osc_type_reports(char a0[])
{
  strcpy(a0,"ihsb");
  return 1;
}

int get_osc_type_accepts(char a0[])
{
  strcpy(a0,"ihsb");
  return 1;
}


#if 0

// This is the TCP server for the OSC transport protocol

void osc_xtcp_handler(chanend tcp_svr, xtcp_connection_t *conn)
{

}

struct osc_connection_state {
  char inbuf[OSC_MAX_MESSAGE_LENGTH];
  char outbuf[OSC_MAX_MESSAGE_LENGTH];
  int active;
};

struct osc_connection_state osc_states[OSC_MAX_CONNECTIONS];


void osc_process_msg(chanend c_xtcp, xtcp_connection_t *conn, 
                     char *data, int len)
{

  if (match) {
    xtcp_buffered_send(c_xtcp, conn, msg, n);    
  }
  else {
    xtcp_ack_recv(c_xtcp, conn);
  }
}

void osc_init_state(tcp_svr, conn) 
{
  osc_connection_state *st = NULL;
  for(int i=0;i<OSC_MAX_CONNECTIONS;i++)
    if (!osc_states[i].active) {
      st = osc_states[i];
      break;
    }

  if (st == NULL) {
    xtcp_abort(c_xtcp, conn);
    return;
  }

  st->active = 1;
  xtcp_buffer_connection_recv(c_xtcp, conn, st->inbuf,
                              OSC_INBUF_SIZE,
                              XTCP_BUFFER_MODE_SLIP);

  xtcp_buffer_connection_send(c_xtcp, conn, st->outbuf, OSC_OUTBUF_SIZE);

  xtcp_set_connection_appstate(c_xtcp, conn, st);
}

// HTTP event handler
void osc_xtcp_handler(chanend tcp_svr, xtcp_connection_t *conn)
{
  char *data;   
  // We have received an event from the TCP stack, so respond appropriately

  // Ignore events that are not directly relevant to http
  switch (conn->event) 
    {
    case XTCP_IFUP:
    case XTCP_IFDOWN:
    case XTCP_ALREADY_HANDLED:
      return;
    }

  if (conn->local_port == XAVBC_SERVER_PORT) {
    switch (conn->event)
      {
      case XTCP_NEW_CONNECTION:
        osc_init_state(tcp_svr, conn);
        break;          
      case XTCP_RECV_DATA:       
        len = xtcp_get_buffered_data(conn, &data);
        if (len > 0) 
          osc_process_msg(c_xtcp, conn, data, len);        
        else
          xtcp_ack_recv(c_xtcp, conn);
        break;        
      case XTCP_SENT_DATA:       
        xtcp_ack_recv(c_xtcp, conn);
        break;
      case XTCP_TIMED_OUT:
      case XTCP_ABORTED:
      case XTCP_CLOSED:
        osc_free_state(conn);
        break;
      }
    conn->event = XTCP_ALREADY_HANDLED;
  }
  return;
}



#endif
