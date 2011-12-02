#include <print.h>
#include "media_input_fifo.h"
#include "avb_1722_def.h"

typedef struct {
	unsigned packet_rd;
	unsigned int fifo[MEDIA_INPUT_FIFO_WORD_SIZE];
} ififo_c_t;

void media_input_fifo_enable_fifos(unsigned int enable)
{
}

void media_input_fifo_disable_fifos(unsigned int enable)
{
}

unsigned int media_input_fifo_enable_ind_state()
{
	return 0;
}

unsigned int media_input_fifo_enable_req_state()
{
	return 0;
}

void media_input_fifo_update_enable_ind_state(unsigned int enable, unsigned int mask)
{
}


void media_input_fifo_init(media_input_fifo_t media_input_fifo0, int stream_num)
{
  volatile ififo_c_t *s =  (ififo_c_t *)media_input_fifo0;
  s->packet_rd = 0;
  return;
}

void media_input_fifo_disable(media_input_fifo_t media_input_fifo0)
{
	media_input_fifo_init(media_input_fifo0, 0);
}

int media_input_fifo_enable(media_input_fifo_t media_input_fifo0,
                             int rate)
{
	return 0;
}

int media_input_fifo_empty(media_input_fifo_t media_input_fifo0)
{
	volatile ififo_c_t *s =  (ififo_c_t *)media_input_fifo0;
	return s->fifo[s->packet_rd+48] == 0;
}

void media_input_fifo_flush(media_input_fifo_t media_input_fifo0)
{
	while (!media_input_fifo_empty(media_input_fifo0)) {
		media_input_fifo_release_packet(media_input_fifo0);
	}
}

unsigned int *
media_input_fifo_get_packet(media_input_fifo_t media_input_fifo0)
{
	ififo_c_t *s =  (ififo_c_t *)media_input_fifo0;
	return &s->fifo[s->packet_rd+1];
}

void 
media_input_fifo_release_packet(media_input_fifo_t media_input_fifo0)
{
	ififo_c_t *s =  (ififo_c_t *)media_input_fifo0;
	s->fifo[s->packet_rd+49] = 0;
	s->packet_rd += (TS_INPUT_PACKET_SIZE/4); // size of one packet structure
	s->packet_rd *= (s->packet_rd < MEDIA_INPUT_FIFO_WORD_SIZE);
}

void
init_media_input_fifos(media_input_fifo_t ififos[],
                       media_input_fifo_data_t ififo_data[],
                       int n)
{
	for(int i=0;i<n;i++) {
		ififos[i] = (unsigned int) &ififo_data[i];
	}
}
