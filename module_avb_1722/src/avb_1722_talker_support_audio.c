/**
 * \file avb_1722_talker_support_audio.c
 * \brief 1722 Talker support C functions
 */
#include "avb_conf.h"
#include "xscope.h"
#include "simple_printf.h"

#ifdef AVB_1722_FORMAT_SAF
#define AVG_PRESENTATION_TIME_DELTA 125000  // 125us = 6 samples at 48kHz
#else
#define AVG_PRESENTATION_TIME_DELTA 166667  // 166.667us = 8 samples at 48kHz
#endif
//#define MAX_PRESENTATION_TIME_DELTA_DELTA AVG_PRESENTATION_TIME_DELTA / 100 // 1% deviation
#define MAX_PRESENTATION_TIME_DELTA_DELTA AVG_PRESENTATION_TIME_DELTA/5 // 200% deviation

//#define AVB_TALKER_DEBUG_LOGIC
//#define PRINT

#if defined(AVB_1722_FORMAT_61883_6) || defined(AVB_1722_FORMAT_SAF)

#define streaming
#include <xccompat.h>
#include <string.h>

#include "avb_1722_talker.h"
#include "gptp.h"
#include "media_input_fifo.h"

// default audio sample type 24bits.
unsigned int AVB1722_audioSampleType = MBLA_24BIT;


/** This configure AVB Talker buffer for a given stream configuration.
 *  It updates the static portion of Ehternet/AVB transport layer headers.
 */
void AVB1722_Talker_bufInit(unsigned char Buf0[],
		avb1722_Talker_StreamConfig_t *pStreamConfig,
		int vlanid)
{
	int i;
	unsigned char *Buf = &Buf0[2];
	AVB_Frame_t *pEtherHdr = (AVB_Frame_t *) &(Buf[0]);
	AVB_DataHeader_t *p1722Hdr = (AVB_DataHeader_t *) &(Buf[AVB_ETHERNET_HDR_SIZE]);
	AVB_AVB1722_CIP_Header_t *p61883Hdr = (AVB_AVB1722_CIP_Header_t *) &(Buf[AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE]);

	unsigned data_block_size;

	// store the sample type
	switch (pStreamConfig->sampleType)
	{
	case MBLA_20BIT:
		AVB1722_audioSampleType = MBLA_20BIT;
		data_block_size = pStreamConfig->num_channels * 1;
		break;
	case MBLA_16BIT:
		AVB1722_audioSampleType = MBLA_16BIT;
		data_block_size = pStreamConfig->num_channels / 2;
		break;
	case MBLA_24BIT:
		AVB1722_audioSampleType = MBLA_24BIT;
		data_block_size = pStreamConfig->num_channels * 1;
		break;
	default:
		//printstr("ERROR: AVB1722_Talker_bufInit : Unsupported audio MBLA type.\n");
		AVB1722_audioSampleType = MBLA_24BIT;
		data_block_size = pStreamConfig->num_channels * 1;
		break;
	}

	// clear all the bytes in header.
	memset( (void *) Buf, 0, (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE));

	//--------------------------------------------------------------------------
	// 1. Initialaise the ethernet layer.
	// copy both Src/Dest MAC address
	for (i = 0; i < MAC_ADRS_BYTE_COUNT; i++) {
		pEtherHdr->DA[i] = pStreamConfig->destMACAdrs[i];
		pEtherHdr->SA[i] = pStreamConfig->srcMACAdrs[i];
	}
	SET_AVBTP_TPID(pEtherHdr, AVB_TPID);
	SET_AVBTP_PCP(pEtherHdr, AVB_DEFAULT_PCP);
	SET_AVBTP_CFI(pEtherHdr, AVB_DEFAULT_CFI);
	SET_AVBTP_VID(pEtherHdr, vlanid);
	SET_AVBTP_ETYPE(pEtherHdr, AVB_ETYPE);

	//--------------------------------------------------------------------------
	// 2. Initialaise the AVB TP layer.
	// NOTE: Since the data structure is cleared before we only set the requird bits.
	SET_AVBTP_SV(p1722Hdr, 1); // set stream ID to valid.
	SET_AVBTP_STREAM_ID0(p1722Hdr, pStreamConfig->streamId[0]);
	SET_AVBTP_STREAM_ID1(p1722Hdr, pStreamConfig->streamId[1]);

	//--------------------------------------------------------------------------
	// 3. Initialise the Simple Audio Format protocol specific part
#if AVB_1722_FORMAT_SAF
	//TODO://This is hardcoded for 48k 32 bit 32 bit samples 2 channels
	SET_AVBTP_PROTOCOL_SPECIFIC(p1722Hdr, 2);
	SET_AVBTP_GATEWAY_INFO(p1722Hdr, 0x02000920);
	SET_AVBTP_SUBTYPE(p1722Hdr, 2);
#else
	//--------------------------------------------------------------------------
	// 3. Initialaise the 61883 CIP protocol specific part
	SET_AVB1722_CIP_TAG(p1722Hdr, AVB1722_DEFAULT_TAG);
	SET_AVB1722_CIP_CHANNEL(p1722Hdr, AVB1722_DEFAULT_CHANNEL);
	SET_AVB1722_CIP_TCODE(p1722Hdr, AVB1722_DEFAULT_TCODE);
	SET_AVB1722_CIP_SY(p1722Hdr, AVB1722_DEFAULT_SY);

	SET_AVB1722_CIP_EOH1(p61883Hdr, AVB1722_DEFAULT_EOH1);
	SET_AVB1722_CIP_SID(p61883Hdr, AVB1722_DEFAULT_SID);
	SET_AVB1722_CIP_DBS(p61883Hdr, data_block_size);

	SET_AVB1722_CIP_FN(p61883Hdr, AVB1722_DEFAULT_FN);
	SET_AVB1722_CIP_QPC(p61883Hdr, AVB1722_DEFAULT_QPC);
	SET_AVB1722_CIP_SPH(p61883Hdr, AVB1722_DEFAULT_SPH);
	SET_AVB1722_CIP_DBC(p61883Hdr, AVB1722_DEFAULT_DBC);

	SET_AVB1722_CIP_EOH2(p61883Hdr, AVB1722_DEFAULT_EOH2);
	SET_AVB1722_CIP_FMT(p61883Hdr, AVB1722_DEFAULT_FMT);
	SET_AVB1722_CIP_FDF(p61883Hdr, AVB1722_DEFAULT_FDF);
	SET_AVB1722_CIP_SYT(p61883Hdr, AVB1722_DEFAULT_SYT);
#endif

}

static void sample_copy_strided(int *src, unsigned int *dest, int stride, int n) {
	int i;
	for (i = 0; i < n; i++) {
#if AVB_1722_FORMAT_SAF
		unsigned sample = *src << 8;
#else
		unsigned sample = (*src & 0xffffff) | AVB1722_audioSampleType;
#endif
		sample = __builtin_bswap32(sample);
		*dest = sample;
		src += 1;
		dest += stride;
	}
}

#ifdef USE_XSCOPE
// globals to store prev values
unsigned prev_ptp_ts=0;
unsigned prev_presentationTime=0;
unsigned prev_valid=0;
int prev_startIndex;

#endif

#ifdef AVB_TALKER_DEBUG_LOGIC
#if(AVB_NUM_SOURCES>4)
#error("Talker Debug Logic breaks the timing at > 4 Talker Streams");
#endif
unsigned prev_chan_presentationTime;
int expected_rdIndex;
unsigned rdIndex_prediction_count=0;
char rdIndex_prediction_valid=0;
#endif

/** This receives user defined audio samples from local out stream and packetize
 *  them into specified AVB1722 transport packet.
 */
int avb1722_create_packet(unsigned char Buf0[],
		avb1722_Talker_StreamConfig_t *stream_info,
		ptp_time_info_mod64 *timeInfo,
		int time)
{
	unsigned int presentationTime = 0;
	int timerValid = 0;
	int i;
	int num_channels = stream_info->num_channels;
	int stream_id0 = stream_info->streamId[0];
	media_input_fifo_t *map = stream_info->map;
	int samples_per_fifo_packet = stream_info->samples_per_fifo_packet;
	int num_audio_samples;
	int samples_in_packet;

	// align packet 2 chars into the buffer so that samples are
	// word align for fast copying.
	unsigned char *Buf = &Buf0[2];

#if AVB_1722_FORMAT_SAF
	unsigned int *dest = (unsigned int *) &Buf[(AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE)];
#else
	unsigned int *dest = (unsigned int *) &Buf[(AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE)];
#endif

	int stride = num_channels;
	unsigned ptp_ts = 0;
	int dbc;
	int pkt_data_length;
    
    AVB_Frame_t *pEtherHdr = (AVB_Frame_t *) &(Buf[0]);
    for (i = 0; i < MAC_ADRS_BYTE_COUNT; i++) {
        pEtherHdr->DA[i] = stream_info->destMACAdrs[i];
    } 

	// Check to see if there is something that can be transmitted.  If there is not, then we give up
	// transmitting this packet, because there may be other streams serviced by this thread which
	// can be serviced.  Since a packet on the wire is always shorter than a packet in the fifo,
	// we know that having a packet in the fifo is enough to transmit one on the wire.
	//
	// There is a slight issue here, that because wire packets are potentially shorter than fifo
	// packets, that we will occasionally not transmit when we could do. The period of this is
	// 1/(ceil(rate/8000)-(rate/8000))
	for (i = 0; i < num_channels; i++) {
	    if (media_input_fifo_empty(map[i])) return 0;
	}

	// If the FIFOs are not being filled then also do not process the packet
	if ((media_input_fifo_enable_ind_state() & stream_info->fifo_mask) == 0) return 0;

	// Figure out if it is time to transmit a packet
	if (!stream_info->transmit_ok) {
		int elapsed = time - stream_info->last_transmit_time;
		if (elapsed < AVB1722_PACKET_PERIOD_TIMER_TICKS)
			return 0;

		stream_info->transmit_ok = 1;
	}

	// Figure out the number of samples in the 1722 packet
	samples_in_packet = stream_info->samples_per_packet_base;

	stream_info->rem += stream_info->samples_per_packet_fractional;

	if (stream_info->rem & 0xffff0000) {
		samples_in_packet += 1;
		stream_info->rem &= 0xffff;
	}

	num_audio_samples = samples_in_packet * num_channels;

#ifdef AVB_1722_FORMAT_SAF
	pkt_data_length = (num_audio_samples << 2);
#else
	pkt_data_length = AVB_CIP_HDR_SIZE + (num_audio_samples << 2);
#endif

	// Find the DBC for the current stream
	dbc = stream_info->dbc_at_start_of_last_fifo_packet;
	dbc += samples_per_fifo_packet - stream_info->samples_left_in_fifo_packet;

	// Get the audio data packet information
	// Note: see 61883-6 section 6.2 which explains that the timestamps coming out
	// of the FIFO should refer to the data blocks which are equal to zero mod
	// the SYT_INTERVAL.  this is implicit in our implementation, because each
	// fifo packet is a group of data blocks, and the first sample, which carries
	// the timestamp is index 0
	if (stream_info->initial != 0) {
		for (i = 0; i < num_channels; i++) {
			int *src = (int *) media_input_fifo_get_packet(map[i], &presentationTime, &(stream_info->dbc_at_start_of_last_fifo_packet));
			media_input_fifo_set_ptr(map[i], src);
		}
		timerValid = 1;
		stream_info->initial = 0;
		dbc = stream_info->dbc_at_start_of_last_fifo_packet;
		stream_info->samples_left_in_fifo_packet = samples_per_fifo_packet;
	} else {
		if (stream_info->samples_left_in_fifo_packet < samples_in_packet) {
			// Not enough samples left in fifo packet to fill the 1722 packet
			// therefore pull out remaining samples and get the next packet
#ifdef USE_XSCOPE_PROBES
		    //xscope_probe(21); // start
#endif
			for (i = 0; i < num_channels; i++) {
				int *src = media_input_fifo_get_ptr(map[i]);
				sample_copy_strided(src, dest, stride, stream_info->samples_left_in_fifo_packet);
				media_input_fifo_release_packet(map[i]);
				src = (int *) media_input_fifo_get_packet(map[i], &presentationTime, &(stream_info->dbc_at_start_of_last_fifo_packet));
				media_input_fifo_set_ptr(map[i], src);
				dest += 1;
#ifdef AVB_TALKER_DEBUG_LOGIC
			   if(i>0 && presentationTime!=prev_chan_presentationTime) {
#ifdef PRINT
				  simple_printf("ERROR: Presentation time for channel %d : %d \n  differs from previous channel time : %d\n",
						  i, presentationTime, prev_chan_presentationTime);
#endif

			   };
			   prev_chan_presentationTime=presentationTime;

#endif
			}

			timerValid = 1;
			dest += (stream_info->samples_left_in_fifo_packet - 1) * num_channels;
			samples_in_packet -= stream_info->samples_left_in_fifo_packet;
			stream_info->samples_left_in_fifo_packet = samples_per_fifo_packet;
#ifdef USE_XSCOPE_PROBES
			//xscope_probe(21);
#endif
#ifdef AVB_TALKER_DEBUG_LOGIC
			if((stream_id0 & 0xF)==0) { // only for stream 0
				volatile ififo_t *media_input_fifo =  (ififo_t *) map[num_channels-1];
                volatile int packetSize;
                if(rdIndex_prediction_valid) {
					if(media_input_fifo->rdIndex != expected_rdIndex) {
#ifdef PRINT
						  simple_printf("ERROR: Expected rdIndex ptr 0x%x differs from actual 0x%x\n",expected_rdIndex, media_input_fifo->rdIndex);
#endif
					}
                }
                expected_rdIndex = media_input_fifo->rdIndex;
                packetSize = media_input_fifo->packetSize*4; // byte address increment
                expected_rdIndex+= packetSize;

			    if (expected_rdIndex + packetSize > media_input_fifo->fifoEnd)
			    	expected_rdIndex = (int *) &media_input_fifo->buf[0]; // wrap
			    rdIndex_prediction_count++;
			    rdIndex_prediction_valid=1;
			}
#endif
		}
	}

	for (i = 0; i < num_channels; i++) {
		int *src = media_input_fifo_get_ptr(map[i]);
		sample_copy_strided(src, dest, stride, samples_in_packet);
		dest += 1;
		media_input_fifo_set_ptr(map[i], src + samples_in_packet);
	}

	stream_info->samples_left_in_fifo_packet -= samples_in_packet;

	dbc &= 0xff;

#if !AVB_1722_FORMAT_SAF
	AVB1722_CIP_HeaderGen(Buf, dbc);
#endif
	// perform required updates to header
	if (timerValid) {
		ptp_ts = local_timestamp_to_ptp_mod32(presentationTime, timeInfo);
		ptp_ts = ptp_ts + stream_info->presentation_delay;

#ifdef USE_XSCOPE
			if((stream_id0 & 0xF)==0) { // only for stream 0
				if(prev_valid) {
				    // trace only for stream 0
					//xscope_probe_data(15, (int) (ptp_ts - prev_ptp_ts));
#ifdef USE_XSCOPE_PROBES
					xscope_probe_data(16, (int) (presentationTime - prev_presentationTime));
					xscope_probe_data(17, (unsigned) (presentationTime));
#endif

#ifdef AVB_TALKER_DEBUG_LOGIC
					int ptp_ts_delta = (int) (ptp_ts - prev_ptp_ts);
					if(ptp_ts_delta > AVG_PRESENTATION_TIME_DELTA+MAX_PRESENTATION_TIME_DELTA_DELTA ||
					   ptp_ts_delta < AVG_PRESENTATION_TIME_DELTA-MAX_PRESENTATION_TIME_DELTA_DELTA) {
#ifdef PRINT
						  simple_printf("ERROR: Expecting ptp_ts (Presentation Time) to change between %d and %d. Actual change %d\n",
								  AVG_PRESENTATION_TIME_DELTA-MAX_PRESENTATION_TIME_DELTA_DELTA,
								  AVG_PRESENTATION_TIME_DELTA+MAX_PRESENTATION_TIME_DELTA_DELTA,
								  ptp_ts_delta);
						  simple_printf("  from fifo values presentationTime %d, prev presentationTime %d\n", presentationTime, prev_presentationTime);
						  //simple_printf("        hex values presentationTime 0x%x, prev presentationTime 0x%x\n", presentationTime, prev_presentationTime);
#endif
					}
#endif

				};
			    prev_ptp_ts = ptp_ts;
			    prev_presentationTime = presentationTime;

			    prev_valid = 1;
			}
#endif
	}

	// Update timestamp value and valid flag.
	AVB1722_AVBTP_HeaderGen(Buf, timerValid, ptp_ts, pkt_data_length, stream_info->sequence_number, stream_id0);

#ifdef BUGFIX_12860
    stream_info->last_transmit_time += AVB1722_PACKET_PERIOD_TIMER_TICKS;
#else
    stream_info->last_transmit_time = time;
#endif
	stream_info->transmit_ok = 0;
	stream_info->sequence_number++;
	return (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + pkt_data_length);
}

#endif
