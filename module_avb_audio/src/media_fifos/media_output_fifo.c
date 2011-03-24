#include <print.h>
#include <xccompat.h>
#include "media_output_fifo.h"
#include "avb_1722_def.h"
#include "media_clock_client.h"
#include "simple_printf.h"

#define OUTPUT_DURING_LOCK 1

// IMPORTANT: This data structure must match the XC-friendly structure
// defined in media_output_fifo.h
typedef struct ofifo_t {
  int zero_flag;
  unsigned int * dptr;
  unsigned int * wrptr;
  unsigned int * end_of_fifo;  
  unsigned int * start_of_fifo;
  unsigned int * marker;
  int local_ts;
  int ptp_ts;
  unsigned int sample_count;
  unsigned int * zero_marker;
  ofifo_state_t state;
  int last_notification_time;
  int stream_num;
  int media_clock;
  unsigned int fifo[MEDIA_OUTPUT_FIFO_SAMPLE_FIFO_SIZE];
  int pending_init_notification;
  unsigned int sample_count_at_timestamp;
} ofifo_t;



void
media_output_fifo_init(int s0, int stream_num) 
{
  struct ofifo_t *s = 
    (struct ofifo_t *) s0;

  s->state = DISABLED;
  s->dptr = s->start_of_fifo;
  s->wrptr = s->start_of_fifo;
  s->start_of_fifo = &s->fifo[0];
  s->end_of_fifo = &s->fifo[MEDIA_OUTPUT_FIFO_SAMPLE_FIFO_SIZE];
  s->media_clock = -1;
  s->stream_num = stream_num;
  s->pending_init_notification = 0;
  s->last_notification_time = 0;
}

void
disable_media_output_fifo(int s0) 
{
  struct ofifo_t *s = 
    (struct ofifo_t *) s0;

  s->state = DISABLED;
  s->zero_flag = 1;
}

void
enable_media_output_fifo(int s0, int media_clock) 
{
  struct ofifo_t *s = 
    (struct ofifo_t *) s0;

  s->state = ZEROING;
  s->dptr = s->start_of_fifo;
  s->wrptr = s->start_of_fifo;
  s->marker = (unsigned int *) 0;
  s->local_ts = 0;
  s->ptp_ts = 0;
  s->zero_marker = s->end_of_fifo-1;
  s->zero_flag = 1;
  *s->zero_marker = 1;
  s->sample_count = 0;
  s->media_clock = media_clock;
  s->pending_init_notification = 1;
  return;
}




int get_media_output_fifo_num(media_output_fifo_t s0)
{
  struct ofifo_t *s = 
    (struct ofifo_t *) s0;
  return s->stream_num;
}

void media_output_fifo_set_ptp_timestamp(media_output_fifo_t s0,
                                         unsigned int timestamp)
{
  struct ofifo_t *s = 
    (struct ofifo_t *) s0;

  if (s->marker == 0 && s->local_ts == 0) {
    s->ptp_ts = timestamp;
    s->marker = s->wrptr;
    s->sample_count_at_timestamp = s->sample_count;
  }

  return;
}

int 
media_output_fifo_get_timestamps(media_output_fifo_t s0,
                                 unsigned int *local_ts,
                                 unsigned int *ptp_ts)
{
  struct ofifo_t *s = 
    (struct ofifo_t *) s0;

  if (s->local_ts != 0 && s->ptp_ts != 0)  {
    *local_ts = s->local_ts;
    *ptp_ts = s->ptp_ts;
    s->local_ts = 0;
    s->ptp_ts = 0;
    return 1;
  }
  
  return 0;
}

unsigned int
media_output_fifo_pull_sample(media_output_fifo_t s0,
                              unsigned int timestamp)
{
  struct ofifo_t *s = 
    (struct ofifo_t *) s0;
  unsigned int sample;
  unsigned int *dptr = s->dptr;
  
  if (dptr == s->wrptr)
    return 0;

  sample = *dptr;
  if (dptr == s->marker && s->local_ts == 0) { 
    if (timestamp)
      s->local_ts = timestamp;
    else
      s->marker = 0;
  }
  dptr++;
  if (dptr == s->end_of_fifo) {
    dptr = s->start_of_fifo;
  }
  
  s->dptr = dptr;

  if (s->zero_flag)
    sample = 0;

  return sample;
}



#define NOTIFICATION_PERIOD 250
//#define NOTIFICATION_PERIOD 1000

void 
media_output_fifo_maintain(media_output_fifo_t s0,
                           chanend buf_ctl,
                           int *notified_buf_ctl)
{
  struct ofifo_t *s = (struct ofifo_t *) s0;
  unsigned time_since_last_notification;
    
  if (s->pending_init_notification && !(*notified_buf_ctl)) {
    notify_buf_ctl_of_new_stream(buf_ctl, s0);
    *notified_buf_ctl = 1;
    s->pending_init_notification = 0;
  }

  switch (s->state) 
    {
    case DISABLED:
      break;
    case ZEROING:
      if (*s->zero_marker == 0) {
        // we have zero-ed the entire fifo
        // set the wrptr so that the fifo size is 3/4 of the buffer size
        int buf_len = (s->end_of_fifo - s->start_of_fifo);
        unsigned int *new_wrptr;
        
        new_wrptr = s->dptr + ((buf_len>>1));
        while (new_wrptr >= s->end_of_fifo) 
          new_wrptr -= buf_len;

        s->wrptr = new_wrptr;
        s->state = LOCKING;
        s->marker = 0;
        s->local_ts = 0;
        s->ptp_ts = 0;
#ifdef OUTPUT_DURING_LOCK
        s->zero_flag = 0;
#endif
      }
      break;
    case LOCKING:
    case LOCKED:
      time_since_last_notification = 
        (signed) s->sample_count - (signed) s->last_notification_time;
      if (s->ptp_ts != 0 && 
          s->local_ts != 0 && 
          !(*notified_buf_ctl) 
          &&
          (s->last_notification_time == 0 ||
           time_since_last_notification > NOTIFICATION_PERIOD)
          )
        {
          notify_buf_ctl_of_info(buf_ctl, s0);
          *notified_buf_ctl = 1;
          s->last_notification_time = s->sample_count;
        }
      break;
    }
}

/*
void 
media_output_fifo_push_sample(media_output_fifo_t s0,
                                  unsigned int sample)

{
  struct ofifo_t *s = (struct ofifo_t *) s0;
  unsigned int *wrptr = s->wrptr;
  unsigned int *new_wrptr;
  if (s->state == ZEROING)
    sample = 0;

  new_wrptr = wrptr+1;
  
  if (new_wrptr == s->end_of_fifo)
    new_wrptr = s->start_of_fifo;
  
  
  if (new_wrptr == s->dptr)
    return;
  
  *wrptr = sample;
  s->wrptr = new_wrptr;
  s->sample_count++;
  return;
}

*/

void 
media_output_fifo_strided_push(media_output_fifo_t s0,
                                   unsigned int *sample_ptr,
                                   int stride,
                                   int n)

{
  struct ofifo_t *s = (struct ofifo_t *) s0;
  unsigned int *wrptr = s->wrptr;
  unsigned int *new_wrptr;
  int i;
  int sample;
  int zero = (s->state != ZEROING);
  int count=0;
  
  for(i=0;i<n;i+=stride) {
    count++;
    sample = *sample_ptr;
    sample = __builtin_bswap32(sample);
    sample_ptr += stride;
    sample = sample * zero;
      
    new_wrptr = wrptr+1;
    
    if (new_wrptr == s->end_of_fifo)
      new_wrptr = s->start_of_fifo;
        
    if (new_wrptr != s->dptr) {
      *wrptr = sample;
      wrptr = new_wrptr;         
    }   
  }

  s->wrptr = wrptr;
  s->sample_count+=count;
  return;
}



void
media_output_fifo_handle_buf_ctl(chanend buf_ctl, 
                                 int s0,
                                 int *buf_ctl_notified)
{
  int cmd;              
  struct ofifo_t *s = (struct ofifo_t *) s0;
  cmd = get_buf_ctl_cmd(buf_ctl); 
  switch (cmd)
    {
    case BUF_CTL_REQUEST_INFO: {
      send_buf_ctl_info(buf_ctl,                         
                        s->state == LOCKED,
                        s->sample_count_at_timestamp,
                        s->ptp_ts,
                        s->local_ts,
                        s->dptr - s->start_of_fifo,
                        s->wrptr - s->start_of_fifo);
      s->ptp_ts = 0;
      s->local_ts = 0;
      s->marker = (unsigned int *) 0;
      break;
    }
    case BUF_CTL_REQUEST_NEW_STREAM_INFO: {
      send_buf_ctl_new_stream_info(buf_ctl,                         
                                   s->media_clock);
      buf_ctl_ack(buf_ctl);      
      *buf_ctl_notified = 0;
      break;
    }
    case BUF_CTL_ADJUST_FILL:
      {
        int adjust;
        unsigned int *new_wrptr;
        adjust = get_buf_ctl_adjust(buf_ctl);

        new_wrptr = s->wrptr - adjust;
        while (new_wrptr < s->start_of_fifo)
          new_wrptr += (s->end_of_fifo - s->start_of_fifo);

        while (new_wrptr >= s->end_of_fifo)
          new_wrptr -= (s->end_of_fifo - s->start_of_fifo);

        s->wrptr = new_wrptr;
      }
      s->state = LOCKED;
      s->zero_flag = 0;
      s->ptp_ts = 0;
      s->local_ts = 0;
      s->marker = (unsigned int *) 0;
      buf_ctl_ack(buf_ctl); 
      *buf_ctl_notified = 0;
      break;
    case BUF_CTL_RESET:
      s->state = ZEROING;
      if (s->wrptr == s->start_of_fifo)
        s->zero_marker = s->end_of_fifo - 1;
      else
        s->zero_marker = s->wrptr - 1;
      s->zero_flag = 1;
      *s->zero_marker = 1;
      buf_ctl_ack(buf_ctl); 
      *buf_ctl_notified = 0;
      break;
    case BUF_CTL_ACK:
      buf_ctl_ack(buf_ctl); 
      *buf_ctl_notified = 0;
      break;
    default:
      printstr("media_output_fifo_buffer: unknown buf_ctl command!\n");
      break;
    }            
  return;
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
