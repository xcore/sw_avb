#include <xs1.h>
#include <platform.h>
#include "avb.h"
#include "xdk_demo_stream_manager.h"
#include "avb_1722_router.h"
#include "LCD_TXT_Client.h"
#include "simple_printf.h"
#include "avb_stream_detect.h"

#define STREAM_TIMEOUT 100000


#define MAX_STREAMS 100

typedef struct stream_table_entry {
  unsigned int hi;
  unsigned int lo;
  unsigned int last_seen;
  unsigned int valid;
  unsigned int vlan;
  unsigned char addr[6];
} stream_table_entry;

extern stream_table_entry stream_table[];

extern int stream_table_num_streams;

// some function to maintain a list of avb streams the demo has found
int AVB1722_AddToStreamTable(unsigned int id_ho,  unsigned int id_lo,
                             unsigned vlan,
                             unsigned char addr[6]);

void xdk_stream_manager_display_table(chanend txt_svr);

stream_table_entry stream_table[MAX_STREAMS];

int stream_table_num_streams = 0;



static unsigned int curStreamId[2] = {0,0};
//static int cur_enabled = 0;
void xdk_manage_streams(chanend listener_config,
                        chanend txt_svr,
                        int display_result,
                        unsigned int &change_stream) 
{
  int res;
  unsigned int streamId[2];
  unsigned vlan;
  unsigned char addr[6];

  // check if there is a new stream
  res = 0;
  res = avb_check_for_new_stream(streamId, vlan, addr);
      
  // if so, add it to the stream table
  if (res) {
    simple_printf("Found %x%x\n.", streamId[0], streamId[1]);
    if (1 || (streamId[0] & 0xffffff00) == 0x00229700) {
      AVB1722_AddToStreamTable(streamId[0],
                               streamId[1],
                               vlan,
                               addr);
    }
    else {
      simple_printf("Ignored\n");
      res = 0;
    }
  }

  if (change_stream || res) {
    int new_hi=0, new_lo=0;
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
            vlan = stream_table[i].vlan;
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
      int map[2];
      
      // we want to change the stream we are listening to
      // map the new stream to our single I/O output
      curStreamId[0] = new_hi;
      curStreamId[1] = new_lo;
      simple_printf("Mapping %x%x ---> %d\n", 
                    curStreamId[0],
                    curStreamId[1],
                    0);

      map[0] = 0; map[1] = 1;
      set_avb_sink_sync(0, 0);
      set_avb_sink_channels(0, 2);
      set_avb_sink_map(0, map, 2);
      set_avb_sink_state(0, AVB_SINK_STATE_DISABLED);
      set_avb_sink_id(0, curStreamId);
      set_avb_sink_vlan(0, vlan);
      set_avb_sink_addr(0, addr, 6);
      set_avb_sink_state(0, AVB_SINK_STATE_POTENTIAL);

      change_stream = 0;
    }
    else 
      if (!res) 
        display_result = 0;

    if (display_result) 
      xdk_stream_manager_display_table(txt_svr);           

  }
}


void xdk_stream_manager_display_table(chanend txt_svr) 
{
  int i=0;
  int num_valid_streams = 0;
  LCD_TXT_ClrLine(txt_svr, 0);
  LCD_TXT_PutStringColour(txt_svr,"           STREAM SELECTION\n",LCD_RED);
  LCD_TXT_PutStringColour(txt_svr,"\n\n\nStreams:\n\n", LCD_BLUE);
  for (i=0;i<stream_table_num_streams;i++) {            
    if (stream_table[i].valid) {

      int col;
      int isCurrent = (stream_table[i].hi == curStreamId[0] &&
                       stream_table[i].lo == curStreamId[1]);
      num_valid_streams++;      

      if (isCurrent)
        col = LCD_GREEN;
      else
        col = LCD_BLUE;
      LCD_TXT_PutStringColour(txt_svr, "        ", col);
      if (stream_table[i].hi == 0xffffffff)
        LCD_TXT_PutStringColour(txt_svr,"     *MIX*      ", col);
      else {
        LCD_TXT_PutHex(txt_svr, stream_table[i].hi, col);
        LCD_TXT_PutHex(txt_svr, stream_table[i].lo, col);
      }
      if (isCurrent)
        LCD_TXT_PutStringColour(txt_svr, "  <-", col);
      LCD_TXT_PutStringColour(txt_svr, "\n", col);
    }
          }
  for (;i<18-6-num_valid_streams;i++) {
    LCD_TXT_PutStringColour(txt_svr, "                         \n", LCD_BLUE);
  }
  return;
}



static int removeOldestStream(unsigned timestamp)
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



int AVB1722_AddToStreamTable(unsigned int id_hi,
                             unsigned int id_lo,
                             unsigned vlan,
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
        invalid_entry = removeOldestStream(timestamp);

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

