/**
 * \file avb_1722_listener_support.c
 * \brief IEC 61883-6/AVB1722 Listener support C functions
 */
#define streaming
#include "avb_1722_listener.h"
#include "avb_1722_common.h"
#include "gptp.h"
#include "avb_1722_def.h"
#include "media_output_fifo.h"
#include <string.h>
#include <xs1.h>
#include "avb_conf.h"
#include "simple_printf.h"
#include "xscope.h"

#if defined(AVB_1722_FORMAT_SAF) || defined(AVB_1722_FORMAT_61883_6)

#ifdef AVB_1722_RECORD_ERRORS
#if(AVB_NUM_SINKS>4)
#error("Listener Debug Logic breaks the timing at > 4 Listener Streams")
#endif
static unsigned avb_1722_listener_dbc_discontinuity = 0;
static unsigned char avb_1722_listener_prev_seq_num[AVB_NUM_SINKS];  // store prev seq_number per stream
static unsigned avb_1722_listener_seq_num_discountinuity[AVB_NUM_SINKS];
static unsigned avb_1722_listener_seq_started[AVB_NUM_SINKS];
#endif

#ifdef USE_XSCOPE
char prev_avbtp_ts_valid=0;
unsigned prev_avbtp_timestamp;
#endif

int avb_1722_listener_process_packet(chanend buf_ctl,
                                     unsigned char Buf0[], 
                                     int numBytes,
                                     avb_1722_stream_info_t *stream_info,
                                     ptp_time_info_mod64* timeInfo,
                                     int index,
                                     int *notified_buf_ctl)
{
   int pktDataLength, dbc_value;
   AVB_DataHeader_t *pAVBHdr;
   AVB_AVB1722_CIP_Header_t *pAVB1722Hdr; 
   unsigned char *Buf = &Buf0[2];
   int avb_ethernet_hdr_size = (Buf[12]==0x81) ? 18 : 14;
   int num_samples_in_payload, num_channels_in_payload;
   pAVBHdr = (AVB_DataHeader_t *) &(Buf[avb_ethernet_hdr_size]);
   pAVB1722Hdr = (AVB_AVB1722_CIP_Header_t *) &(Buf[avb_ethernet_hdr_size + AVB_TP_HDR_SIZE]);
   unsigned char *sample_ptr;
   int i;
   int num_channels = stream_info->num_channels;
   media_output_fifo_t *map = &stream_info->map[0];
   int stride;   
#if !AVB_1722_FORMAT_SAF
  int dbc_diff;
#endif

   // sanity check on number bytes in payload
#if AVB_1722_FORMAT_SAF
   if (numBytes <= avb_ethernet_hdr_size + AVB_TP_HDR_SIZE)
#else
   if (numBytes <= avb_ethernet_hdr_size + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE)
#endif
   {
      return (0);
   }   
   if (AVBTP_CD(pAVBHdr) != AVBTP_CD_DATA)
   {
      return (0);      
   }
   if (AVBTP_SV(pAVBHdr) == 0)
   {
      return (0);            
   }

#ifdef AVB_1722_RECORD_ERRORS
   {
 	   // log discontinuities in seq_number per stream
       int unique_stream_idx = stream_info->unique_idx;
 	   if(unique_stream_idx >= AVB_NUM_SINKS) {
 		   simple_printf("ERROR: unique_stream_idx %x > AVB_NUM_SINKS %x\n", unique_stream_idx, AVB_NUM_SINKS);
 	   } else {
 		 if(!avb_1722_listener_seq_started[unique_stream_idx]) {
 			avb_1722_listener_seq_started[unique_stream_idx] = 1;
 		    avb_1722_listener_prev_seq_num[unique_stream_idx] = (unsigned char) AVBTP_SEQUENCE_NUMBER(pAVBHdr);  // init prev
 		 } else if((unsigned char) AVBTP_SEQUENCE_NUMBER(pAVBHdr) != (unsigned char) (avb_1722_listener_prev_seq_num[unique_stream_idx]+1)) {
 		   avb_1722_listener_seq_num_discountinuity[unique_stream_idx]++;
 		   unsigned num_disc = avb_1722_listener_seq_num_discountinuity[unique_stream_idx];
 		   if((num_disc%0x1000)==0) {
 			  simple_printf("ERROR: Stream %d: Detected %d seq_number discontinuities so far\n", unique_stream_idx, num_disc);
 		   }
 		   // may break timing:
 		   //simple_printf("ERROR: Stream %d: seq_number discontinuity. prev_seq_num %d, seq_num %d\n", unique_stream_idx, (unsigned char) avb_1722_listener_prev_seq_num[unique_stream_idx],AVBTP_SEQUENCE_NUMBER(pAVBHdr));
	 	   avb_1722_listener_prev_seq_num[unique_stream_idx] = (unsigned char) AVBTP_SEQUENCE_NUMBER(pAVBHdr); // re-init
 		 } else {
	 	   avb_1722_listener_prev_seq_num[unique_stream_idx]++; // PASS, just increment
 		 }
 	   }
    }
#endif

#if !AVB_1722_FORMAT_SAF
   dbc_value = (int) pAVB1722Hdr->DBC;
   dbc_diff = dbc_value - stream_info->dbc;
   stream_info->dbc = dbc_value;

   if (dbc_diff < 0) dbc_diff += 0x100;
#endif

   pktDataLength = NTOH_U16(pAVBHdr->packet_data_length);
#if AVB_1722_FORMAT_SAF
 #if AVB_1722_FORMAT_SAF16
   num_samples_in_payload = pktDataLength>>1; // two octets hold one 16-bit sample
 #else
   num_samples_in_payload = pktDataLength>>2;
 #endif
#else
   num_samples_in_payload = (pktDataLength-8)>>2;
#endif

   int prev_num_samples = stream_info->prev_num_samples;
   stream_info->prev_num_samples = num_samples_in_payload;

   if (stream_info->chan_lock < 16)
   {
     int num_channels;

#if !AVB_1722_FORMAT_SAF
   if (!prev_num_samples || dbc_diff == 0)
       return 0;
#endif
     
#if AVB_1722_FORMAT_SAF
     num_channels = AVBTP_PROTOCOL_SPECIFIC(pAVBHdr);
#else
     num_channels = prev_num_samples / dbc_diff;
#endif
     
     if (!stream_info->num_channels_in_payload || 
         stream_info->num_channels_in_payload != num_channels)
     {
       stream_info->num_channels_in_payload = num_channels;
       stream_info->chan_lock = 0;
       stream_info->rate = 0;
     }

     stream_info->rate += num_samples_in_payload;
       
     stream_info->chan_lock++;

#if !AVB_1722_FORMAT_SAF
     if (stream_info->chan_lock == 16)
     {
    	 stream_info->rate = (stream_info->rate / stream_info->num_channels_in_payload / 16);

    	 switch (stream_info->rate)
    	 {
    		 case 1: stream_info->rate = 8000; break;
    		 case 2: stream_info->rate = 16000; break;
    		 case 4: stream_info->rate = 32000; break;
    		 case 5: stream_info->rate = 44100; break;
    		 case 6: stream_info->rate = 48000; break;
    		 case 11: stream_info->rate = 88200; break;
    		 case 12: stream_info->rate = 96000; break;
    		 default: stream_info->rate = 0; break;
    	 }
     }
#endif

     return 0;
   }
#if(AVB_1722_RECORD_ERRORS && !AVB_1722_FORMAT_SAF)
   else if (dbc_diff != num_samples_in_payload) {
	   avb_1722_listener_dbc_discontinuity++;
   }
#endif

#if AVB_1722_FORMAT_SAF
   if ((AVBTP_TV(pAVBHdr)==1)) {
	   unsigned sample_num = 0;
#else
   if ((AVBTP_TV(pAVBHdr)==1)) {
	   // See 61883-6 section 6.2 which explains that the receiver can calculate
	   // which data block (sample) the timestamp refers to using the formula:
	   //   index = (SYT_INTERVAL - dbc % SYT_INTERVAL) % SYT_INTERVAL

	   unsigned syt_interval, sample_num;

	   switch (stream_info->rate)
	   {
		   case 8000: 	syt_interval = 1; break;
		   case 16000: 	syt_interval = 2; break;
		   case 32000:	syt_interval = 8; break;
		   case 44100:	syt_interval = 8; break;
		   case 48000:	syt_interval = 8; break;
		   case 88200:	syt_interval = 16; break;
		   case 96000:	syt_interval = 16; break;
		   case 176400:	syt_interval = 32; break;
		   case 192000:	syt_interval = 32; break;
		   default: break;
	   }
	   sample_num = (syt_interval - (dbc_value & (syt_interval-1))) & (syt_interval-1);
#endif
	   // register timestamp
	   for (int i=0;i<num_channels;i++)  {
		   media_output_fifo_set_ptp_timestamp(map[i], AVBTP_TIMESTAMP(pAVBHdr), sample_num);
	   }
#ifdef USE_XSCOPE_PROBES
	   if((AVBTP_STREAM_ID0(pAVBHdr)&0xF) == 0) { // reduce probing to workaround xscope issue
		  if(prev_avbtp_ts_valid) {
	         xscope_probe_data(8, (signed) (AVBTP_TIMESTAMP(pAVBHdr) - prev_avbtp_timestamp));
		  }
		  prev_avbtp_timestamp = AVBTP_TIMESTAMP(pAVBHdr);
		  prev_avbtp_ts_valid = 1;
	   }
#endif
   }

  for (i=0;i<num_channels;i++)  {
    media_output_fifo_maintain(map[i], buf_ctl, notified_buf_ctl);
  }           


  // now send the samples
#if AVB_1722_FORMAT_SAF
  sample_ptr = (unsigned char *) &Buf[(avb_ethernet_hdr_size +
                                      AVB_TP_HDR_SIZE)];
#else
  sample_ptr = (unsigned char *) &Buf[(avb_ethernet_hdr_size + 
                                      AVB_TP_HDR_SIZE +
                                      AVB_CIP_HDR_SIZE)];
#endif

   num_channels_in_payload = stream_info->num_channels_in_payload;
 
   stride = num_channels_in_payload;

   num_channels = 
     num_channels < num_channels_in_payload ? 
     num_channels :
     num_channels_in_payload;

   //simple_printf("stride %d, num_samples_in_payload %d\n", stride, num_samples_in_payload);
  for(i=0;i<num_channels;i++) {
#if AVB_1722_FORMAT_SAF16
    media_output_fifo_strided_push_saf16(map[i], (unsigned short *) sample_ptr,
                                   stride, num_samples_in_payload);
    sample_ptr += 2; // packed 16-bit samples in audio_data_payload
#else
    media_output_fifo_strided_push(map[i], (unsigned int *) sample_ptr,
                                   stride, num_samples_in_payload);
    sample_ptr += 4;
#endif
  }

   return(1);
}

#endif


