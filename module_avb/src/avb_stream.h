#ifndef __avb_stream_h__
#define __avb_stream_h__

#include "xc2compat.h"
#include <xccompat.h>
#include "avb_conf.h"

typedef struct avb_srp_info_t {
  unsigned stream_id[2];
  unsigned char dest_mac_addr[6];
  short vlan_id;
  short tspec_max_frame_size;
  short tspec_max_interval;
  unsigned char tspec;
  unsigned accumulated_latency;
} avb_srp_info_t;

typedef struct avb_stream_info_t
{
	int state;
	mrp_attribute_state *unsafe srp_talker_attr0;
	mrp_attribute_state *unsafe srp_talker_attr1;
	mrp_attribute_state *unsafe srp_talker_failed_attr;
	mrp_attribute_state *unsafe srp_listener_attr0;
	mrp_attribute_state *unsafe srp_listener_attr1;
	char tile_id;
	char local_id;
	char num_channels;
	char format;
	int rate;
	char sync;
	short flags;
} avb_stream_info_t;

typedef struct avb_source_info_t
{
	avb_srp_info_t reservation;
	avb_stream_info_t stream;
	chanend *unsafe talker_ctl;
	int presentation;
	int map[AVB_MAX_CHANNELS_PER_TALKER_STREAM];
} avb_source_info_t;


typedef struct avb_sink_info_t
{
	avb_srp_info_t reservation;
	avb_stream_info_t stream;
	chanend *unsafe listener_ctl;
	int map[AVB_MAX_CHANNELS_PER_LISTENER_STREAM];
} avb_sink_info_t;

/** Utility function to get the index of a source stream based on its
 * pointer.  This is used by SRP, which stores a pointer to the stream
 * structure rather than an index.
 */
unsigned avb_get_source_stream_index_from_pointer(avb_source_info_t *unsafe p);

/** Utility function to get the index of a sink stream based on its
 * pointer.  This is used by SRP, which stores a pointer to the stream
 * structure rather than an index.
 */
unsigned avb_get_sink_stream_index_from_pointer(avb_sink_info_t *unsafe p);

unsigned avb_get_source_stream_index_from_stream_id(unsigned int stream_id[2]);
unsigned avb_get_sink_stream_index_from_stream_id(unsigned int stream_id[2]);

#endif // __avb_stream_h__
