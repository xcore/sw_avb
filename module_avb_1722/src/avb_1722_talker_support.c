/*
 * @ModuleName IEC 61883-6/AVB1722 Audio over 1722 AVB Transport.
 * @Description: Implements Talker functionality.
 *
 *
 *
 */
#define streaming 
#include "avb_1722_talker.h"
#include "gptp.h"
#include "media_input_fifo.h"
#include <string.h>
#include <print.h>
#include <xccompat.h>
#include "simple_printf.h"

// Min frame size for sanity check.
#define MIN_ETHERNET_FRAME_SIZE     (60)

#define INVALID_DBC                 (0xF)

// default audio sample type 24bits.
unsigned int AVB1722_audioSampleType = MBLA_24BIT;


/** This generate required CIP Header with specified DBC value.
 *  NOTE: Most fields in function is constants, hence, could be improved for speed.
 *
 *  \para   buf[] buffer array to be populated.
 *  \para   startOffset start byte offset within the buffer for 61883 CIP Header.
 *  \para   dbc DBC value of CIP header to be populated.
 *  \return none.
 */
static inline void AVB1722_CIP_HeaderGen(unsigned char Buf[], int dbc)
{
   AVB_AVB1722_CIP_Header_t *pAVB1722Hdr = (AVB_AVB1722_CIP_Header_t *) &(Buf[AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE]);

   SET_AVB1722_CIP_DBC(pAVB1722Hdr, dbc);      
}

static inline void AVB1722_AVBTP_HeaderGen(unsigned char Buf[], 
                                           int valid_ts,
                                           unsigned avbtp_ts,
                                           int numAudioSamples, 
                                           const unsigned stream_id0)
{
   int pkt_data_length;
   AVB_DataHeader_t *pAVBHdr = (AVB_DataHeader_t *) &(Buf[AVB_ETHERNET_HDR_SIZE]);
   
   // work out the packet data length
   // **** WARNING ****
   // Assume FOUR bytes is used to represent one audio sample, regardless of
   // actual sample bits, valid for 61883-6/AVB1722
   pkt_data_length = 8 + (numAudioSamples << 2);
   HTON_U16(pAVBHdr->packet_data_length, pkt_data_length);
   
   // only stamp the AVBTP timestamp when required.
   if (valid_ts) {
      SET_AVBTP_TV(pAVBHdr, 1);        // AVB timestamp valid.     
      SET_AVBTP_TIMESTAMP(pAVBHdr, avbtp_ts); // Valid ns field.
   } else {
      SET_AVBTP_TV(pAVBHdr, 0);        // AVB timestamp not valid.
      SET_AVBTP_TIMESTAMP(pAVBHdr, 0); // NULL the timestmap field as well.
   }
   
   // update stream ID by adding stream number to preloaded stream ID
   // (ignoring the fact that talkerStreamIdExt is stored MSB-first - it's just an ID)
   SET_AVBTP_STREAM_ID0(pAVBHdr, stream_id0);


}



/** This populates the Ethernet frame header of PTP payload.
 *  NOTE: 
 *  1. All fields in function is constants, hence, could be improved for speed.
 *  2. Source & Destination MAC address can be set via AVB1722_SetMACAdrs(..)
 *     
 *  \para   buf[] buffer array to be populated.
 */
/*
static void AVB1722_Ethernet_HeaderGen(unsigned char Buf[])
{
   // Nothing to do.
}
*/


void avb1722_set_buffer_vlan(int vlan,
                             unsigned char Buf[])
{
   AVB_Frame_t *pEtherHdr    = (AVB_Frame_t *) &(Buf[0]);

   SET_AVBTP_VID(pEtherHdr, vlan);

   return;
}

/** This configure AVB Talker buffer for a given stream configuration.
 *  It updates the static portion of Ehternet/AVB transport layer headers.
 */
void AVB1722_Talker_bufInit(unsigned char Buf0[], 
                            avb1722_Talker_StreamConfig_t *pStreamConfig,
                            int vlanid) 
{
   int i;
   unsigned char *Buf = &Buf0[2];
   AVB_Frame_t *pEtherHdr    = (AVB_Frame_t *) &(Buf[0]);
   AVB_DataHeader_t *pAVBHdr = (AVB_DataHeader_t *) &(Buf[AVB_ETHERNET_HDR_SIZE]);
   AVB_AVB1722_CIP_Header_t *pAVB1722Hdr = (AVB_AVB1722_CIP_Header_t *) &(Buf[AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE]);
   
   
   // store the sample type
   switch (pStreamConfig->sampleType)
   {
      case MBLA_20BIT:
         AVB1722_audioSampleType = MBLA_20BIT;               
         break;
      case MBLA_16BIT:
         AVB1722_audioSampleType = MBLA_16BIT;               
         break;         
      case MBLA_24BIT:
         AVB1722_audioSampleType = MBLA_24BIT;
         break;
      default: 
         //printstr("ERROR: AVB1722_Talker_bufInit : Unsupported audio MBLA type.\n");
         AVB1722_audioSampleType = MBLA_24BIT;         
         break;
   }
   
   // clear all the bytes in header.
   memset( (void *) Buf, 0, (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_AVB1722_HDR_SIZE));
   
   //--------------------------------------------------------------------------
   // 1. Initilaise the ethernet layer.
   // copy both Src/Dest MAC address
   for (i = 0; i < MAC_ADRS_BYTE_COUNT; i ++)
   {
      pEtherHdr->DA[i] = pStreamConfig->destMACAdrs[i];
      pEtherHdr->SA[i] = pStreamConfig->srcMACAdrs[i];      
   }
   SET_AVBTP_TPID(pEtherHdr, AVB_TPID);
   SET_AVBTP_PCP(pEtherHdr, AVB_DEFAULT_PCP);
   SET_AVBTP_CFI(pEtherHdr, AVB_DEFAULT_CFI);
   SET_AVBTP_VID(pEtherHdr, vlanid);
   SET_AVBTP_ETYPE(pEtherHdr, AVB_ETYPE);
   
   //--------------------------------------------------------------------------
   // 2. Initilaise the AVB TP layer.
   // NOTE: Since the data structure is cleared before we only set the requird bits.
   SET_AVBTP_SV(pAVBHdr, 1);  // set stream ID to valid.
   SET_AVBTP_STREAM_ID0(pAVBHdr, pStreamConfig->streamId[0]);
   SET_AVBTP_STREAM_ID1(pAVBHdr, pStreamConfig->streamId[1]);
   // set pkt data length.
   SET_AVBTP_PACKET_DATA_LENGTH(pAVBHdr, AVB1722_DEFAULT_AVB_PKT_DATA_LENGTH);
   
   //--------------------------------------------------------------------------
   // 3. Initilaise the 61883 CIP Header
   SET_AVB1722_CIP_TAG(pAVB1722Hdr, AVB1722_DEFAULT_TAG); 
   SET_AVB1722_CIP_CHANNEL(pAVB1722Hdr, AVB1722_DEFAULT_CHANNEL);
   SET_AVB1722_CIP_TCODE(pAVB1722Hdr, AVB1722_DEFAULT_TCODE);
   SET_AVB1722_CIP_SY(pAVB1722Hdr, AVB1722_DEFAULT_SY);
   
   SET_AVB1722_CIP_EOH1(pAVB1722Hdr, AVB1722_DEFAULT_EOH1);
   SET_AVB1722_CIP_SID(pAVB1722Hdr, AVB1722_DEFAULT_SID);
   SET_AVB1722_CIP_DBS(pAVB1722Hdr, AVB1722_DEFAULT_DBS);
   
   SET_AVB1722_CIP_FN(pAVB1722Hdr, AVB1722_DEFAULT_FN);
   SET_AVB1722_CIP_QPC(pAVB1722Hdr, AVB1722_DEFAULT_QPC);
   SET_AVB1722_CIP_SPH(pAVB1722Hdr, AVB1722_DEFAULT_SPH);   
   SET_AVB1722_CIP_DBC(pAVB1722Hdr, AVB1722_DEFAULT_DBC);
   
   SET_AVB1722_CIP_EOH2(pAVB1722Hdr, AVB1722_DEFAULT_EOH2);
   SET_AVB1722_CIP_FMT(pAVB1722Hdr, AVB1722_DEFAULT_FMT);
   SET_AVB1722_CIP_FDF(pAVB1722Hdr, AVB1722_DEFAULT_FDF);
   SET_AVB1722_CIP_SYT(pAVB1722Hdr, AVB1722_DEFAULT_SYT);
   
   //printstr("INFO: AVB1722_Talker_bufInit: Done..\n");
}


static void sample_copy_strided(int *src,
                                unsigned int *dest,
                                int stride,
                                int n)
{
  int i;
  for(i=0;i<n;i++) {
    unsigned sample = (*src & 0xffffff) | 0x01000000;
    sample = __builtin_bswap32(sample);
    *dest = sample;
    src += 1;
    dest += stride;
  }
}
                   
#define DEBUG_DBC 1

#ifdef DEBUG_DBC             
static int dbc_miss = 0;
#endif

/** This receives user defined audio samples from local out stream and packetize
 *  them into specified AVB1722 transport packet.
 */
int avb1722_create_packet(unsigned char Buf0[],
                         avb1722_Talker_StreamConfig_t *stream_info,
                          ptp_time_info_mod64 *timeInfo,
                          int time)
{
   unsigned int presentationTime;
   int pktSize;
   int timerValid;
   int i;
   int num_channels = stream_info->num_channels;
   media_input_fifo_t *map = stream_info->map;
   int stream_id0 = stream_info->streamId[0];
   int samples_per_fifo_packet = stream_info->samples_per_fifo_packet;
   int num_audio_samples;
   int samples_in_packet;
   // pull the required samples out of the fifo

   // align packet 2 chars into the buffer so that samples are
   // word align for fast copying.
   unsigned char *Buf = &Buf0[2];

   unsigned int *dest = (unsigned int *) &Buf[(AVB_ETHERNET_HDR_SIZE + 
                                               AVB_TP_HDR_SIZE + 
                                               AVB_AVB1722_HDR_SIZE)];
   int stride = num_channels;
   unsigned ptp_ts=0;
   int dbc;
   if (media_input_fifo_empty(map[0])) 
     return 0;

   if (!stream_info->transmit_ok) {
     int elapsed = time - stream_info->last_transmit_time;
     if (elapsed < AVB1722_PACKET_PERIOD_TIMER_TICKS) 
       return 0;
     
     stream_info->transmit_ok = 1;
   }

   if (stream_info->latency == 0) {
     int ticks_per_sample = 100000000 / (stream_info->samples_per_fifo_packet * 8000);
     stream_info->latency = (media_input_fifo_fill_level(map[0]) - stream_info->samples_per_fifo_packet)* ticks_per_sample;
     if (stream_info->latency == 0) 
       stream_info->latency = 1;
   }

   samples_in_packet = stream_info->samples_per_packet_base;

   stream_info->rem += stream_info->samples_per_packet_fractional;

   if (stream_info->rem & 0xffff0000) {
     samples_in_packet += 1;
     stream_info->rem &= 0xffff;
   }

   num_audio_samples = samples_in_packet * num_channels;

   dbc = stream_info->dbc;  

   dbc += samples_per_fifo_packet - stream_info->samples_left;

   if (stream_info->samples_left < samples_in_packet) {
     for (i=0;i<num_channels;i++) {
       int *src = media_input_fifo_get_ptr(map[i]);   
       if (stream_info->initial) {
         stream_info->initial = 0;
       }
       else {
         sample_copy_strided(src, dest , stride, stream_info->samples_left);
         media_input_fifo_release_packet(map[i]);
       }
       src = (int *) media_input_fifo_get_packet(map[i],
                                                 &presentationTime,
                                                 &(stream_info->dbc));       
       media_input_fifo_set_ptr(map[i],src);   
       dest+=1;
     }

#ifdef DEBUG_DBC
     if (stream_info->prev_dbc != -1 && 
         (
          (stream_info->prev_dbc + samples_per_fifo_packet) 
          != stream_info->dbc
          )          
         )           
       {
         dbc_miss++;          
         if (dbc_miss > 1) 
         {
           //           simple_printf("%d,%d\n",stream_info->prev_dbc,stream_info->dbc);
           dbc_miss = 0;
         }
       }
     stream_info->prev_dbc = stream_info->dbc;     
#endif

     dest += (stream_info->samples_left - 1) * num_channels;
     samples_in_packet -= stream_info->samples_left;
     stream_info->samples_left = samples_per_fifo_packet;
   }

   for (i=0;i<num_channels;i++) {     
     int *src = media_input_fifo_get_ptr(map[i]);      
     sample_copy_strided(src, dest , stride, samples_in_packet);
     dest += 1;
     media_input_fifo_set_ptr(map[i], src+samples_in_packet);
   }

   stream_info->samples_left -= samples_in_packet;

   dbc &= 0xff;

   AVB1722_CIP_HeaderGen(Buf, dbc);     


   timerValid = 1;

   // perform required updates to header
   if (timerValid) {
     ptp_ts = local_timestamp_to_ptp_mod32(presentationTime, timeInfo);
     ptp_ts = ptp_ts + stream_info->presentation_delay;
   } 
     
   // Update timestamp value and valid flag.
   AVB1722_AVBTP_HeaderGen(Buf, 
                           timerValid,
                           ptp_ts,
                           num_audio_samples, 
                           stream_id0);
   
      
   pktSize =  
     AVB_ETHERNET_HDR_SIZE +
     AVB_TP_HDR_SIZE + 
     AVB_AVB1722_HDR_SIZE +
     (num_audio_samples<<2);

   stream_info->last_transmit_time = time;     
   stream_info->transmit_ok = 0;      
   return (pktSize);
}


 
