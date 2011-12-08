#include <print.h>
#include <xccompat.h>
#include <xscope.h>
#include "media_output_fifo.h"

// State bits
#define OFIFO_STATE_ENABLED (0x01)


// IMPORTANT: This data structure must match the XC-friendly structure
// defined in media_output_fifo.h
typedef struct {
	unsigned state;
	unsigned packet_wr;
	unsigned fifo[MEDIA_OUTPUT_FIFO_WORD_SIZE];
} ofifo_c_t;



void
media_output_fifo_init(int s0, unsigned stream_num)
{
  ofifo_c_t *s = (ofifo_c_t *) s0;

  s->state = 0;
  s->packet_wr = 0;
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
  s->packet_wr = 0;
  return;
}


// 1722 thread
void 
media_output_fifo_push(media_output_fifo_t s0, unsigned int *sample_ptr, int index, int n)

{
	ofifo_c_t *s = (ofifo_c_t *) s0;
	unsigned *dst = &s->fifo[s->packet_wr + index];
	for (int i=0; i<n; i++) {
		*dst++ = *sample_ptr++;
	}

	if (n + index == 8) {
		s->packet_wr += n;
		s->packet_wr *= (s->packet_wr < MEDIA_OUTPUT_FIFO_WORD_SIZE);
	}
}


void
init_media_output_fifos(media_output_fifo_t ofifos[],
						media_output_fifo_data_t ofifo_data[],
						int n)
{
  for(int i=0;i<n;i++) {
    ofifos[i] = (unsigned int) &ofifo_data[i];
  }
}
