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
#include "simple_printf.h"
#include <string.h>
#include <print.h>
#include <xs1.h>
#include "avb_conf.h"

#ifdef AVB_1722_RECORD_ERRORS
static unsigned avb_1722_listener_dbc_discontinuity = 0;
#endif

int avb_1722_listener_process_packet(chanend buf_ctl,
                                     unsigned char Buf0[], 
                                     int numBytes,
                                     avb_1722_stream_info_t *stream_info,
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
#if !AVB_1722_SAF
  int dbc_diff;
#endif

   // sanity check on number bytes in payload
#if AVB_1722_SAF
   if (numBytes <= avb_ethernet_hdr_size + AVB_TP_HDR_SIZE)
#else
   if (numBytes <= avb_ethernet_hdr_size + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE)
#endif
   {
      printstr("ERROR: 1722 Invalid packet size.\n");     
      return (0);
   }   
   if (AVBTP_CD(pAVBHdr) != AVBTP_CD_DATA)
   {
      printstr("ERROR: 1722 Command packet.\n");
      return (0);      
   }
   if (AVBTP_SV(pAVBHdr) == 0)
   {
      printstr("ERROR: 1722 Invalid streamId.\n");
      return (0);            
   }


#if !AVB_1722_SAF
   dbc_value = (int) pAVB1722Hdr->DBC;
   dbc_diff = dbc_value - stream_info->dbc;
   stream_info->dbc = dbc_value;

   if (dbc_diff < 0) dbc_diff += 0x100;
#endif

   pktDataLength = NTOH_U16(pAVBHdr->packet_data_length);
#if AVB_1722_SAF
   num_samples_in_payload = pktDataLength>>2;
#else
   num_samples_in_payload = (pktDataLength-8)>>2;
#endif

   int prev_num_samples = stream_info->prev_num_samples;
   stream_info->prev_num_samples = num_samples_in_payload;

   stream_info->count++;

   if (stream_info->chan_lock < 10) {
     int num_channels;

#if !AVB_1722_SAF
   if (!prev_num_samples || dbc_diff == 0)
       return 0;
#endif
     
#if AVB_1722_SAF
     num_channels = AVBTP_PROTOCOL_SPECIFIC(pAVBHdr);
#else
     num_channels = prev_num_samples / dbc_diff;
#endif
     
     if (!stream_info->num_channels_in_payload || 
         stream_info->num_channels_in_payload != num_channels)  {
       stream_info->num_channels_in_payload = num_channels;
       stream_info->chan_lock = 0;
     }
       
     stream_info->chan_lock++;

     return 0;
   }
#ifdef AVB_1722_RECORD_ERRORS
   else if (dbc_diff != num_samples_in_payload) {
	   avb_1722_listener_dbc_discontinuity++;
   }
#endif


#if AVB_1722_SAF
   if ((AVBTP_TV(pAVBHdr)==1)) {
#else
   if ((AVBTP_TV(pAVBHdr)==1) && (dbc_value & 7)==0) {
#endif
	   unsigned int timestamp = AVBTP_TIMESTAMP(pAVBHdr);

	   // register timestamp
	  if (timestamp != 0) {
		  int i;
		  for (i=0;i<num_channels;i++)  {
			  media_output_fifo_set_ptp_timestamp(map[i], timestamp);
		  }
	  }
   }

  for (i=0;i<num_channels;i++)  {
    media_output_fifo_maintain(map[i], buf_ctl, notified_buf_ctl);
  }           


  // now send the samples
#if AVB_1722_SAF
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

  for(i=0;i<num_channels;i++) {
    media_output_fifo_strided_push(map[i], (unsigned int *) sample_ptr, 
                                   stride, num_samples_in_payload);
    sample_ptr += 4;
  }

   return(1);
}




