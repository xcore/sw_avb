#include <avb_conf.h>
#include <stdlib.h>
#ifndef AVB_STREAM_DETECT_HISTORY_SIZE
#define AVB_STREAM_DETECT_HISTORY_SIZE (AVB_NUM_SINKS*4)
#endif

struct stream_info {
  unsigned id[2];
  unsigned vlan;
  char addr[6];
};


static struct stream_info stream_history[AVB_STREAM_DETECT_HISTORY_SIZE];
static int rdPtr=0;
static int wrPtr = 0;

void avb_add_detected_stream(unsigned streamId[2], 
                             unsigned vlan,
                             unsigned char addr[6],
                             int addr_offset)
{  
  int found = 0;

  if (AVB_STREAM_DETECT_HISTORY_SIZE == 0)
    return;

  for(int i=0;i<AVB_STREAM_DETECT_HISTORY_SIZE;i++)
    if (streamId[0] == stream_history[i].id[0] &&
        streamId[1] == stream_history[i].id[1]) {
      found = 1;
      break;
    }

  if (!found) {
    int new_wrPtr = wrPtr + 1;
    if (new_wrPtr==AVB_STREAM_DETECT_HISTORY_SIZE)
      new_wrPtr = 0;
    if (new_wrPtr==rdPtr) {
      // fifo is full, drop oldest
      rdPtr++;
      if (rdPtr==AVB_STREAM_DETECT_HISTORY_SIZE)
        rdPtr = 0;
    }
    
    stream_history[wrPtr].id[0] = streamId[0];
    stream_history[wrPtr].id[1] = streamId[1];
    stream_history[wrPtr].vlan = vlan;
    
    if (addr != NULL) {
      unsigned long long x;
      
      for(int i=0;i<6;i++) 
        x = (x<<8) + addr[i];

      x += addr_offset;

      for(int i=5;i>=0;i--) {
        stream_history[wrPtr].addr[i] = x & 0xff;
        x>>=8;
      }
    }
    else
      for(int i=0;i<6;i++)
        stream_history[wrPtr].addr[i] = 0;
      
    wrPtr = new_wrPtr;
  }
  
  return;
}

int avb_check_for_new_stream(unsigned streamId[2], unsigned *vlan,
                             unsigned char addr[6])
{
  if (AVB_STREAM_DETECT_HISTORY_SIZE == 0)
    return 0;

  if (rdPtr != wrPtr) {
    streamId[0] = stream_history[rdPtr].id[0];
    streamId[1] = stream_history[rdPtr].id[1];
    *vlan = stream_history[rdPtr].vlan;
    for (int i=0;i<6;i++)
      addr[i] = stream_history[rdPtr].addr[i];
    rdPtr++;
    if (rdPtr==AVB_STREAM_DETECT_HISTORY_SIZE)
      rdPtr = 0;    
    return 1;
  }
  return 0;
}
