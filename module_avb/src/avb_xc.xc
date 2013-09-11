#include <xs1.h>
#include "avb.h"
#include <print.h>

void avb_get_control_packet(chanend c_rx,
                            unsigned int buf[],
                            unsigned int &nbytes)
{
  unsigned int src_port;
  safe_mac_rx(c_rx,
              (buf, unsigned char[]),
              nbytes,
              src_port,
              MAX_AVB_CONTROL_PACKET_SIZE);
}

int avb_register_listener_streams(chanend listener_ctl,
                                   int num_streams)
{
  int core_id;
  int link_id;
  core_id = get_local_tile_id();
  listener_ctl <: core_id;
  listener_ctl <: num_streams;
  listener_ctl :> link_id;
  return link_id;
}

void avb_register_talker_streams(chanend talker_ctl,
                                 int num_streams)
{
  int core_id;
  core_id = get_local_tile_id();
  talker_ctl <: core_id;
  talker_ctl <: num_streams;
}
