#include <xs1.h>
#include <platform.h>
#include <print.h>
#include "avb.h"
#include "demo_stream_manager.h"
#include "simple_printf.h"

#define STREAM_TIMEOUT 100000

#define MAX_STREAMS 20

typedef struct stream_table_entry {
  unsigned int hi;
  unsigned int lo;
  unsigned int vlan;
  unsigned char addr[6];
  unsigned int last_seen;
  unsigned int valid;
} stream_table_entry;

// some function to maintain a list of avb streams the demo has found
static int add_to_stream_table(unsigned int id_ho,  unsigned int id_lo,
                               unsigned vlan,
                               unsigned char addr[6]);

stream_table_entry stream_table[MAX_STREAMS];

int stream_table_num_streams = 0;

static unsigned int curStreamId[2] = {0,0};

void demo_manage_listener_stream(unsigned int &change_stream,
                                 int selected_chan) 
{
  int res=0;
  unsigned int streamId[2];
  unsigned vlan;
  unsigned char addr[6];
  int channel = selected_chan;

  // check if there is a new stream
  res = avb_check_for_new_stream(streamId, vlan, addr);
      
  // if so, add it to the stream table
  if (res) {
    simple_printf("Found %x%x\n.", streamId[0], streamId[1]);
    add_to_stream_table(streamId[0], streamId[1], vlan, addr);
  }

  if (change_stream || res) {
    int new_hi=0, new_lo=0, new_vlan = 0;
    int getNext = 0;
    int i;         
    
    // first work out what the next stream to change to is (the 
    // one in the table after the currently chosen stream)
    for (i=0;i<stream_table_num_streams;i++) {
      if (stream_table[i].valid) {
        if (new_hi == 0
            ||
            getNext) {
          new_hi = stream_table[i].hi;
          new_lo = stream_table[i].lo;
          new_vlan = stream_table[i].vlan;
          for (int i=0;i<6;i++)
            addr[i] = stream_table[i].addr[i];
          getNext = 0;
        }
        
        if (stream_table[i].hi == curStreamId[0] &&
            stream_table[i].lo == curStreamId[1])
          getNext = 1;
      }
    }
    
    if (change_stream && new_hi != 0) {
      int map[8] = {-1, -1, -1, -1, 0, 1, 2, 3};
      
      // we want to change the stream we are listening to
      // map the new stream to our single I/O output
      curStreamId[0] = new_hi;
      curStreamId[1] = new_lo;
      simple_printf("Mapping %x%x ---> %d\n", 
                    curStreamId[0],
                    curStreamId[1],
                    0);

      set_avb_sink_sync(0, 0);
      set_avb_sink_channels(0, 8);
      set_avb_sink_map(0, map, 8);
      set_avb_sink_state(0, AVB_SINK_STATE_DISABLED);
      set_avb_sink_id(0, curStreamId);
      set_avb_sink_vlan(0, new_vlan);
      set_avb_sink_addr(0, addr, 6);
      set_avb_sink_state(0, AVB_SINK_STATE_POTENTIAL);

      change_stream = 0;
    }

  }
}



static int remove_oldest_stream(unsigned timestamp)
{
  int oldest = 0;
  int cur_diff = 0;
  for (int i=0;i<stream_table_num_streams;i++)
    {
      int diff = (int) timestamp - (int) stream_table[i].last_seen;

      if (diff > cur_diff) {
        cur_diff = diff;
        oldest = i;
      }
    }

  stream_table[oldest].valid = 0;
  return oldest;
}



static int add_to_stream_table(unsigned int id_hi,
                               unsigned int id_lo,
                               unsigned int vlan,
                               unsigned char addr[6])
{
  unsigned timestamp;
  int invalid_entry = -1;
  int seen = 0;
  timer tmr;
  int index = 0;

  tmr :> timestamp;

  for (int i=0;i<stream_table_num_streams;i++)
    {
      if (stream_table[i].hi == id_hi &&
          stream_table[i].lo == id_lo) {
        index = i;
        stream_table[i].valid = 1;
        stream_table[i].last_seen = timestamp;
        seen = 1;
      }

      if (stream_table[i].valid == 0)
        invalid_entry = i;

    }

  if (!seen) {

    if (invalid_entry == -1) {

      if (stream_table_num_streams < MAX_STREAMS-1)
        {
          invalid_entry = stream_table_num_streams;
          stream_table_num_streams++;
        }
      else
        invalid_entry = remove_oldest_stream(timestamp);

    }

    stream_table[invalid_entry].hi = id_hi;
    stream_table[invalid_entry].lo = id_lo;
    stream_table[invalid_entry].vlan = vlan;
    for (int i=0;i<6;i++)
      stream_table[invalid_entry].addr[i] = addr[i];

    stream_table[invalid_entry].last_seen = timestamp;
    stream_table[invalid_entry].valid = 1;
    index = invalid_entry;
  }

  return index;
}

