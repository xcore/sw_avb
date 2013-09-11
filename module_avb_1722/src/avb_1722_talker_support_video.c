/**
 * \file avb_1722_talker_support_video.c
 * \brief 1722 Talker support C functions
 */
#include "avb_conf.h"

#if defined(AVB_1722_FORMAT_61883_4)

#define streaming
#include <xccompat.h>
#include <string.h>

#include "avb_1722_talker.h"
#include "gptp.h"
#include "media_input_fifo.h"

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
	SET_AVBTP_ETYPE(pEtherHdr, AVB_1722_ETHERTYPE);

	//--------------------------------------------------------------------------
	// 2. Initialaise the AVB TP layer.
	// NOTE: Since the data structure is cleared before we only set the requird bits.
	SET_AVBTP_SV(p1722Hdr, 1); // set stream ID to valid.
	SET_AVBTP_STREAM_ID0(p1722Hdr, pStreamConfig->streamId[0]);
	SET_AVBTP_STREAM_ID1(p1722Hdr, pStreamConfig->streamId[1]);

	//--------------------------------------------------------------------------
	// 3. Initialaise the 61883 CIP protocol specific part
	SET_AVB1722_CIP_TAG(p1722Hdr, AVB1722_DEFAULT_TAG);
	SET_AVB1722_CIP_CHANNEL(p1722Hdr, AVB1722_DEFAULT_CHANNEL);
	SET_AVB1722_CIP_TCODE(p1722Hdr, AVB1722_DEFAULT_TCODE);
	SET_AVB1722_CIP_SY(p1722Hdr, AVB1722_DEFAULT_SY);

	SET_AVB1722_CIP_EOH1(p61883Hdr, AVB1722_DEFAULT_EOH1);
	SET_AVB1722_CIP_SID(p61883Hdr, AVB1722_DEFAULT_SID);
	SET_AVB1722_CIP_DBS(p61883Hdr, 6);

	SET_AVB1722_CIP_FN(p61883Hdr, 3);
	SET_AVB1722_CIP_QPC(p61883Hdr, 0);
	SET_AVB1722_CIP_SPH(p61883Hdr, 1);
	SET_AVB1722_CIP_DBC(p61883Hdr, AVB1722_DEFAULT_DBC);

	SET_AVB1722_CIP_EOH2(p61883Hdr, AVB1722_DEFAULT_EOH2);
	SET_AVB1722_CIP_FMT(p61883Hdr, 0x20);
	SET_AVB1722_CIP_FDF(p61883Hdr, 0x80);
	SET_AVB1722_CIP_SYT(p61883Hdr, AVB1722_DEFAULT_SYT);
}



/** This receives user defined audio samples from local out stream and packetize
 *  them into specified AVB1722 transport packet.
 */
int avb1722_create_packet(unsigned char Buf0[],
		avb1722_Talker_StreamConfig_t *stream_info,
		ptp_time_info_mod64 *timeInfo,
		int time)
{
	int stream_id0 = stream_info->streamId[0];
	media_input_fifo_t *map = stream_info->map;

	// align packet 2 chars into the buffer so that samples are
	// word align for fast copying.
	unsigned char *Buf = &Buf0[2];
	unsigned int *dest = (unsigned int *) &Buf[(AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE)];

	int dbc = stream_info->dbc_at_start_of_last_fifo_packet;
	int pkt_data_length = 0;

	// Check to see if there is something that can be transmitted.  If there is not, then we give up
	// transmitting this packet, because there may be other streams serviced by this thread which
	// can be serviced.  Since a packet on the wire is always shorter than a packet in the fifo,
	// we know that having a packet in the fifo is enough to transmit one on the wire.
	//
	// There is a slight issue here, that because wire packets are potentially shorter than fifo
	// packets, that we will occasionally not transmit when we could do. The period of this is
	// 1/(ceil(rate/8000)-(rate/8000))
    if (media_input_fifo_empty(map[0])) return 0;

	// Figure out if it is time to transmit a packet
	if (!stream_info->transmit_ok) {
		int elapsed = time - stream_info->last_transmit_time;
		if (elapsed < AVB1722_PACKET_PERIOD_TIMER_TICKS)
			return 0;

		stream_info->transmit_ok = 1;
	}


	// For all of the packets in the FIFO
	for (unsigned n=0; n<MAX_TS_PACKETS_PER_1722 && !media_input_fifo_empty(map[0]); ++n) {
		unsigned * data = media_input_fifo_get_packet(map[0]);

		// Adjust the timestamp from local to PTP
		*dest++ = local_timestamp_to_ptp_mod32(data[0], timeInfo) + stream_info->presentation_delay;

		// Copy the whole TS packet
		for (unsigned c=1; c<(192/4); ++c) {
			*dest++ = data[c];
		}

		// Update length
		pkt_data_length += 192;
		stream_info->dbc_at_start_of_last_fifo_packet += 8;

		media_input_fifo_release_packet(map[0]);
	}

	AVB1722_CIP_HeaderGen(Buf, dbc);

	// Update timestamp value and valid flag.
	AVB1722_AVBTP_HeaderGen(Buf, 0, 0, pkt_data_length, stream_info->sequence_number, stream_id0);

	stream_info->last_transmit_time = time;
	stream_info->transmit_ok = 0;
	stream_info->sequence_number++;

	return (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE + pkt_data_length);
}

#endif
