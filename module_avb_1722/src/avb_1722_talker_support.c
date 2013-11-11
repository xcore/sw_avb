/**
 * \file avb_1722_talker_support.c
 * \brief 1722 Talker support C functions
 */
#define streaming

#include <xccompat.h>

#include "avb_1722_talker.h"
#include "avb_conf.h"

void AVB1722_CIP_HeaderGen(unsigned char Buf[], int dbc)
{
	AVB_AVB1722_CIP_Header_t *pAVB1722Hdr = (AVB_AVB1722_CIP_Header_t *) &(Buf[AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE]);

	SET_AVB1722_CIP_DBC(pAVB1722Hdr, dbc);
}

void AVB1722_AVBTP_HeaderGen(unsigned char Buf[],
		int valid_ts,
		unsigned avbtp_ts,
		int pkt_data_length,
		int sequence_number,
		const unsigned stream_id0)
{
	AVB_DataHeader_t *pAVBHdr = (AVB_DataHeader_t *) &(Buf[AVB_ETHERNET_HDR_SIZE]);

	SET_AVBTP_PACKET_DATA_LENGTH(pAVBHdr, pkt_data_length);

	// only stamp the AVBTP timestamp when required.
	if (valid_ts) {
		SET_AVBTP_TV(pAVBHdr, 1); // AVB timestamp valid.
		SET_AVBTP_TIMESTAMP(pAVBHdr, avbtp_ts); // Valid ns field.
	} else {
		SET_AVBTP_TV(pAVBHdr, 0); // AVB timestamp not valid.
		SET_AVBTP_TIMESTAMP(pAVBHdr, 0); // NULL the timestmap field as well.
	}

	// update stream ID by adding stream number to preloaded stream ID
	// (ignoring the fact that talkerStreamIdExt is stored MSB-first - it's just an ID)
	SET_AVBTP_STREAM_ID0(pAVBHdr, stream_id0);

	// update the ...
	SET_AVBTP_SEQUENCE_NUMBER(pAVBHdr, sequence_number);
}


void avb1722_set_buffer_vlan(int vlan,
		unsigned char Buf[])
{
	AVB_Frame_t *pEtherHdr = (AVB_Frame_t *) &(Buf[0]);

	SET_AVBTP_VID(pEtherHdr, vlan);

	return;
}

