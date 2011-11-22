#include <print.h>
#include <xccompat.h>
#include <xscope.h>
#include "media_output_fifo.h"

// State bits
#define OFIFO_STATE_ENABLED (0x01)

// Markers for start and end
#define START_OF_FIFO(s) ((unsigned int*)&((s)->fifo[0]))
#define END_OF_FIFO(s)   ((unsigned int*)&((s)->fifo[TS_PACKET_SIZE*TS_FIFO_SIZE]))

// IMPORTANT: This data structure must match the XC-friendly structure
// defined in media_output_fifo.h
typedef struct {
	unsigned state;
	unsigned* packet_wr;
	unsigned* packet_rd;
	unsigned fifo[MEDIA_OUTPUT_FIFO_WORD_SIZE];
} ofifo_c_t;



void
media_output_fifo_init(int s0, unsigned stream_num)
{
  ofifo_c_t *s = (ofifo_c_t *) s0;

  s->state = 0;
  s->packet_wr = START_OF_FIFO(s);
  s->packet_rd = START_OF_FIFO(s);
}

void
disable_media_output_fifo(int s0) 
{
  ofifo_c_t *s = (ofifo_c_t *) s0;

  s->state &= ~OFIFO_STATE_ENABLED;
}

void
enable_media_output_fifo(int s0, int media_clock) 
{
  ofifo_c_t *s = (ofifo_c_t *) s0;

  s->state |= OFIFO_STATE_ENABLED;
  s->packet_wr = START_OF_FIFO(s);
  s->packet_rd = START_OF_FIFO(s);
  return;
}


// 1722 thread
void media_output_fifo_set_ptp_timestamp(media_output_fifo_t s0,
                                         unsigned int ptp_ts,
                                         unsigned sample_number)
{
}


// 1722 thread
void 
media_output_fifo_maintain(media_output_fifo_t s0,
                           chanend buf_ctl,
                           int *notified_buf_ctl)
{
}

// 1722 thread
void 
media_output_fifo_strided_push(media_output_fifo_t s0,
                                   unsigned int *sample_ptr,
                                   int stride,
                                   int n)

{
}


// 1722 thread
void
media_output_fifo_handle_buf_ctl(chanend buf_ctl, 
                                 int s0,
                                 int *buf_ctl_notified)
{
}


void
init_media_output_fifos(media_output_fifo_t ofifos[],
						ofifo_t ofifo_data[],
						int n)
{
  for(int i=0;i<n;i++) {
    ofifos[i] = (unsigned int) &ofifo_data[i];
  }
}
