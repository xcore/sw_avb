#include <xs1.h>
#include "gptp.h"
#include "gptp_cmd.h"
#include "ethernet_rx_client.h"
#include "mac_custom_filter.h"
#include "get_core_id_from_chanend.h"
#include "print.h"

/* These functions are the workhorse functions for the actual protocol.
   They are implemented in gptp.c  */
void ptp_init(chanend, enum ptp_server_type stype);
void ptp_recv(chanend, unsigned char buf[], unsigned ts);
void ptp_periodic(chanend, unsigned);
void ptp_server_set_legacy_mode(int legacy_mode);
void ptp_get_reference_ptp_ts_mod_64(unsigned &hi, unsigned &lo);
void ptp_current_grandmaster(char grandmaster[8]);

#define MAX_PTP_MESG_LENGTH 100

#define PTP_PERIODIC_TIME (10000)  // 0.1 milliseconds

#pragma select handler
void receive_ptp_cmd(chanend c, unsigned int &cmd)
{
  cmd = inuchar(c);
  return;
}

extern unsigned ptp_reference_local_ts;
extern ptp_timestamp ptp_reference_ptp_ts;
extern signed int g_ptp_adjust;
extern signed int g_inv_ptp_adjust;

#define do_ptp_server(c_rx, c_tx, client, num_clients) \
  case ptp_recv_and_process_packet(c_rx, c_tx): \
       break;                     \
  case (int i=0;i<num_clients;i++) ptp_process_client_request(client[i]): \
       break; \
  case ptp_timer when timerafter(ptp_timeout) :> void: \
       ptp_periodic(c_tx, ptp_timeout); \
       ptp_timeout += PTP_PERIODIC_TIME; \
       break

timer ptp_timer;   
unsigned ptp_timeout;

void ptp_server_init(chanend c_rx, chanend c_tx, 
                     enum ptp_server_type server_type)
{                                             

  mac_set_custom_filter(c_rx, MAC_FILTER_PTP);

  ptp_timer :> ptp_timeout;
 
  ptp_init(c_tx, server_type);

}

#pragma select handler
void ptp_recv_and_process_packet(chanend c_rx, chanend c_tx)
{
  unsigned int buf[MAX_PTP_MESG_LENGTH/4];  
  unsigned ts;
  unsigned src_port;
  unsigned len;
  safe_mac_rx_timed(c_rx, 
                   (buf, unsigned char[]), 
                   len,
                   ts, 
                   src_port, 
                   MAX_PTP_MESG_LENGTH);
  ptp_recv(c_tx, (buf, unsigned char[]), ts);
}

static void ptp_give_requested_time_info(chanend c)
{
  int thiscore_now;
  unsigned core_id = get_core_id_from_chanend(c);
  master {
    ptp_timer :> thiscore_now;
    c <: thiscore_now;
    c <: ptp_reference_local_ts;
    c <: ptp_reference_ptp_ts;
    c <: g_ptp_adjust;
    c <: g_inv_ptp_adjust;
    c <: core_id;
  }
}

#pragma select handler
void ptp_process_client_request(chanend c)
{
  unsigned char cmd;
  unsigned thiscore_now;
  unsigned core_id = get_core_id_from_chanend(c);

  cmd = inuchar(c);
  (void) inuchar(c);
  (void) inuchar(c); 
  (void) inct(c);
  switch (cmd) 
    {
    case PTP_GET_TIME_INFO:
      ptp_give_requested_time_info(c);
      break;
    case PTP_GET_TIME_INFO_MOD64: {
      unsigned int hi, lo;
      ptp_get_reference_ptp_ts_mod_64(hi,lo);
      master {
      c :> int;
      ptp_timer :> thiscore_now;
      c <: thiscore_now;
      c <: ptp_reference_local_ts;
      c <: hi;
      c <: lo;
      c <: g_ptp_adjust;
      c <: g_inv_ptp_adjust;
      c <: core_id;
      }                        
      break;
    }
    case PTP_SET_LEGACY_MODE: {
      int mode;
      c :> mode;
      ptp_server_set_legacy_mode(mode);                          
      break;            
    }
    case PTP_GET_GRANDMASTER: {
    	char grandmaster[8];
    	ptp_current_grandmaster(grandmaster);
    	master
    	{
    		for(int i = 0; i < 8; i++)
			{
				c <: grandmaster[i];
			}
    	}
    	break;
    }
    }
}


void ptp_server(chanend c_rx, chanend c_tx, 
                chanend client[], int num_clients,
                enum ptp_server_type server_type)
{

  ptp_server_init(c_rx, c_tx, server_type);

  while (1) {
    select 
      {
        do_ptp_server(c_rx, c_tx, client, num_clients);
      }
  }
}
