/**
 * \file avb_1722_listener_support.c
 * \brief IEC 61883-6/AVB1722 Listener support C functions
 */
#define streaming
#include <xs1.h>
#include <string.h>

#include "avb_1722_listener.h"
#include "avb_1722_common.h"
#include "gptp.h"
#include "avb_1722_def.h"
#include "media_output_fifo.h"
#include "avb_conf.h"

#if defined (AVB_1722_FORMAT_61883_4)

#ifdef AVB_1722_RECORD_ERRORS
static unsigned avb_1722_listener_dbc_discontinuity = 0;
static unsigned avb_1722_sequence_failure = 0;
#endif

int avb_1722_listener_process_packet(chanend buf_ctl,
                                     unsigned char Buf0[],
                                     int numBytes,
                                     avb_1722_stream_info_t *stream_info,
                                     ptp_time_info_mod64* timeInfo,
                                     int index,
                                     int *notified_buf_ctl)
{
	int pktDataLength;
	AVB_DataHeader_t *p1722Hdr;
	AVB_AVB1722_CIP_Header_t *p61886Hdr;
	unsigned char *Buf = &Buf0[2];
	int avb_ethernet_hdr_size = (Buf[12]==0x81) ? 18 : 14;
	int num_blocks_in_payload;
	p1722Hdr = (AVB_DataHeader_t *) &(Buf[avb_ethernet_hdr_size]);
	p61886Hdr = (AVB_AVB1722_CIP_Header_t *) &(Buf[avb_ethernet_hdr_size + AVB_TP_HDR_SIZE]);
	unsigned *sample_ptr = 0;
	media_output_fifo_t *map = &stream_info->map[0];
	int dbc;

	// sanity check on number bytes in payload
	if (numBytes <= avb_ethernet_hdr_size + AVB_TP_HDR_SIZE) return (0);
	if (AVBTP_CD(p1722Hdr) != AVBTP_CD_DATA) return (0);
	if (AVBTP_SV(p1722Hdr) == 0) return (0);

	dbc = (int) p61886Hdr->DBC;

	// If this is the start of a TS souce packet then ok, else we should discard this packet
	// if it is not the next sequence
	switch (stream_info->state) {
	case 0:
		// Initialise sequence number tracker
		stream_info->last_sequence = p1722Hdr->sequence_number;
		stream_info->state = 1;
		return 1;
	case 1:
		// Check for valid sequence number increase
		if ((char)p1722Hdr->sequence_number != (char)(stream_info->last_sequence + 1)) {
			stream_info->state = 0;
			return 1;
		}
		stream_info->last_sequence = p1722Hdr->sequence_number;

		// Hunt for start of TS source packet
		if ((dbc & 0x7) == 0) {
			stream_info->state = 2;
		}
		break;
	case 2:
		// Check for valid sequence number increase
		if ((char)p1722Hdr->sequence_number != (char)(stream_info->last_sequence + 1)) {
#ifdef AVB_1722_RECORD_ERRORS
			avb_1722_sequence_failure++;
#endif
			stream_info->state = 0;
			return 1;
		}
		stream_info->last_sequence = p1722Hdr->sequence_number;
		break;
	}

	pktDataLength = NTOH_U16(p1722Hdr->packet_data_length);
	num_blocks_in_payload = pktDataLength / 24;

	// 61883-4 says there can only certain numbers of blocks in a packet, depending on the
	// DBC.  If the DBC is start of a whole 192 byte TS packet, then there can by any number of
	// whole TS packets in the 61883-4 packet.  If the DBC is not the start of a TS packet, therefore
	// there was previously a 61883-4 packet containing a part of a TS packet, then this 61883-4
	// packet can contain at most the remainder of this TS packet.  In fact, it can contain only
	// a number of blocks as listed below
	//
	//    DBC             Number of 6 byte blocks
	//   -----------------------------------------
	//    xxxxx000               any number
	//    xxxxx001                        1
	//    xxxxx010                        2
	//    xxxxx011                        1
	//    xxxxx100                        4
	//    xxxxx101                        1
	//    xxxxx110                        2
	//    xxxxx111                        1

	sample_ptr = (unsigned*) &Buf[(avb_ethernet_hdr_size + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE)];

	if (num_blocks_in_payload == 0) {
		return 1;
	}
	else if ((num_blocks_in_payload & 0x7) == 0) {
		if ((dbc & 0x7) == 0) {
			// Multiple blocks

			for (int i=0; i<num_blocks_in_payload/8; ++i) {

				// Convert timestamp to local time
				*sample_ptr = ptp_mod32_timestamp_to_local(*sample_ptr, timeInfo);

				media_output_fifo_push(map[0], sample_ptr, 0, 8);
				sample_ptr += 48;
			}
		}
		stream_info->prev_num_samples = 0;
	} else {
		switch (num_blocks_in_payload) {
		case 1:
		case 2:
		case 4:
			if (dbc & (num_blocks_in_payload-1)) {
				num_blocks_in_payload = 0;
			}
			break;
		default:
			// All other cases are not allowed
			num_blocks_in_payload = 0;
		}

		if ((dbc & 0x7) == 0) {
			// Convert timestamp to local time
			*sample_ptr = ptp_mod32_timestamp_to_local(*sample_ptr, timeInfo);
		}

		media_output_fifo_push(map[0], sample_ptr, dbc & 0x7, num_blocks_in_payload);
	}


	return 1;
}

#endif


