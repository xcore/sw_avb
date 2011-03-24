#ifndef __avb_stream_h__
#define __avb_stream_h__

typedef struct avb_source_info_t
{
  int state;
  int srp_attr;
  int core_id;
  int local_id;
  int talker_ctl;
  int num_channels;
  int map[AVB_MAX_CHANNELS_PER_STREAM];
  int format;
  int rate;
  unsigned char dest[6];
  int presentation;
  int sync;
  int vlan;
  char name[AVB_MAX_NAME_LEN];
} avb_source_info_t;


typedef struct avb_sink_info_t
{
  unsigned int streamId[2];
  int state;
  int srp_attr;
  int core_id;
  int local_id;
  int listener_ctl;
  int num_channels;
  int map[AVB_MAX_CHANNELS_PER_STREAM];
  int format;
  int rate;
  int sync;
  int vlan;
  unsigned char addr[6];
  char name[AVB_MAX_NAME_LEN];
} avb_sink_info_t;


#endif // __avb_stream_h__
