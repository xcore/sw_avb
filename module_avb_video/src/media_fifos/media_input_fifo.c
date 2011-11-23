#include <print.h>
#include "media_input_fifo.h"
#include "hwlock.h"
#include "avb_1722_def.h"

static hwlock_t enable_lock;
unsigned int enable_request_state = 0;
unsigned int enable_indication_state = 0;

typedef struct {
	unsigned state;
	unsigned packet_wr;
	unsigned packet_rd;
	unsigned int fifo[MEDIA_INPUT_FIFO_WORD_SIZE];
} ififo_c_t;

void media_input_fifo_enable_fifos(unsigned int enable)
{
	if (!enable_lock) return;
	__hwlock_acquire(enable_lock);
	enable_request_state |= enable;
	__hwlock_release(enable_lock);
}

void media_input_fifo_disable_fifos(unsigned int enable)
{
	if (!enable_lock) return;
	__hwlock_acquire(enable_lock);
	enable_request_state &= ~enable;
	__hwlock_release(enable_lock);
}

unsigned int media_input_fifo_enable_ind_state()
{
	return enable_indication_state;
}

unsigned int media_input_fifo_enable_req_state()
{
	return enable_request_state;
}

void media_input_fifo_update_enable_ind_state(unsigned int enable, unsigned int mask)
{
	if (!enable_lock) return;
	__hwlock_acquire(enable_lock);
	enable_indication_state = (enable_indication_state & ~mask) | enable;
	__hwlock_release(enable_lock);
}


void media_input_fifo_init(media_input_fifo_t media_input_fifo0, int stream_num)
{
  volatile ififo_c_t *s =  (ififo_c_t *)media_input_fifo0;
  s->state = 0;
  s->packet_wr = 0;
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
	return s->packet_wr == s->packet_rd;
}

void media_input_fifo_flush(media_input_fifo_t media_input_fifo0)
{
	volatile ififo_c_t *s =  (ififo_c_t *)media_input_fifo0;
	s->packet_rd = s->packet_wr;
}

unsigned int *
media_input_fifo_get_packet(media_input_fifo_t media_input_fifo0)
{
	volatile ififo_c_t *s =  (ififo_c_t *)media_input_fifo0;
	return &s->fifo[s->packet_rd];
}

void 
media_input_fifo_release_packet(media_input_fifo_t media_input_fifo0)
{
	volatile ififo_c_t *s =  (ififo_c_t *)media_input_fifo0;
	s->packet_rd += (192/4); // size of one packet plus timestamp
}

void
init_media_input_fifos(media_input_fifo_t ififos[],
                       media_input_fifo_data_t ififo_data[],
                       int n)
{
	enable_lock = __hwlock_init();
	for(int i=0;i<n;i++) {
		ififos[i] = (unsigned int) &ififo_data[i];
	}
}
