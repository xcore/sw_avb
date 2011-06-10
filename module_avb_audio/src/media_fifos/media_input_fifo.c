#include "media_input_fifo.h"
#include "avb_1722_def.h"
#include "simple_printf.h"

void media_input_fifo_init(media_input_fifo_t media_input_fifo0, int stream_num)
{
  volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;

  media_input_fifo->sampleCountInPacket = -1;
  media_input_fifo->packetSize = -1;
  return;
}

void media_input_fifo_disable(media_input_fifo_t media_input_fifo0)
{
	media_input_fifo_init(media_input_fifo0, 0);
}

int media_input_fifo_enable(media_input_fifo_t media_input_fifo0,
                             int rate)
{
  volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;
  int packetSize;

  packetSize = ((rate+(AVB1722_PACKET_RATE-1))/AVB1722_PACKET_RATE);
  media_input_fifo->rdIndex = (int) &media_input_fifo->buf[0]; 
  media_input_fifo->wrIndex = (int) &media_input_fifo->buf[0]; 
  media_input_fifo->startIndex = (int) &media_input_fifo->buf[0]; 
  // add on 2 for the dbc and timestamp
  media_input_fifo->dbc = 0;
  media_input_fifo->fifoEnd = (int) &media_input_fifo->buf[MEDIA_INPUT_FIFO_SAMPLE_FIFO_SIZE-1];
  media_input_fifo->sampleCountInPacket = 0;
  media_input_fifo->ptr = 0;
  media_input_fifo->packetSize = packetSize + 2;
  return packetSize;
}

void media_input_fifo_push_sample(media_input_fifo_t media_input_fifo0,
                                  unsigned int sample,
                                  unsigned int ts)
{
  volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;
  int sampleCountInPacket = media_input_fifo->sampleCountInPacket;
  const int packetSize = media_input_fifo->packetSize;
  int* wrIndex;

  if (sampleCountInPacket == -1)
    return;

  if (sampleCountInPacket == 0) {
    // beginning of packet
    wrIndex = (int *) media_input_fifo->startIndex;
    int spaceLeft = ((int *) media_input_fifo->rdIndex) - wrIndex;

    spaceLeft &= (MEDIA_INPUT_FIFO_SAMPLE_FIFO_SIZE-1);
    
    if (spaceLeft && (spaceLeft < media_input_fifo->packetSize)) return;

    *wrIndex = ts;
    wrIndex++;
    *wrIndex = media_input_fifo->dbc;
    wrIndex++;
    *wrIndex = sample;
    wrIndex++;
    media_input_fifo->wrIndex = (int) wrIndex;

    sampleCountInPacket = 3;
  }
  else {
    wrIndex = (int *) media_input_fifo->wrIndex;
    *wrIndex = sample;
    wrIndex++;

    sampleCountInPacket++;
  }

  if (sampleCountInPacket == packetSize) {
    // end of packet
    if ((wrIndex + packetSize) > (int *) media_input_fifo->fifoEnd)
    	wrIndex = (int *) &media_input_fifo->buf[0];

    media_input_fifo->startIndex = (int) wrIndex;
    media_input_fifo->dbc += packetSize-2;
    sampleCountInPacket = 0;
  }

  media_input_fifo->sampleCountInPacket = sampleCountInPacket;
  media_input_fifo->wrIndex = (int) wrIndex;
}

int media_input_fifo_empty(media_input_fifo_t media_input_fifo0)
{
 volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;
 return (media_input_fifo->rdIndex==0 ||
         media_input_fifo->rdIndex==media_input_fifo->startIndex);
}

void media_input_fifo_flush(media_input_fifo_t media_input_fifo0)
{
	volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;
	int *rdIndex = (int *) media_input_fifo->rdIndex;

	if (rdIndex==0) return;

	while (rdIndex != (int *) media_input_fifo->startIndex)
	{
		media_input_fifo_release_packet(media_input_fifo0);
	}

}

unsigned int *
media_input_fifo_get_packet(media_input_fifo_t media_input_fifo0,
                            unsigned int *ts,
                            unsigned int *dbc)
{
  volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;
  int *rdIndex = (int *) media_input_fifo->rdIndex;

  while (media_input_fifo->rdIndex==0);

  rdIndex = (int *) media_input_fifo->rdIndex;

  while (rdIndex == (int *) media_input_fifo->startIndex);


  *ts = *rdIndex;
  *dbc = *(rdIndex+1);

  return ((unsigned int *) (rdIndex+2));
}

void 
media_input_fifo_release_packet(media_input_fifo_t media_input_fifo0)
{
  volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;
  int packetSize;
  int *rdIndex = (int *) media_input_fifo->rdIndex;
  packetSize = media_input_fifo->packetSize;
  rdIndex += packetSize;
  if (rdIndex + packetSize > (int *) media_input_fifo->fifoEnd)
    rdIndex = (int *) &media_input_fifo->buf[0];
  
  media_input_fifo->rdIndex = (int) rdIndex;
  return;
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

extern inline void 
media_input_fifo_set_ptr(media_input_fifo_t media_infput_fifo0,
                         int *p);

extern inline int *
media_input_fifo_get_ptr(media_input_fifo_t media_infput_fifo0);
