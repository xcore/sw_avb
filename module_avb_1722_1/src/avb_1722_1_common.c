#include "avb_1722_common.h"
#include "avb_1722_1_common.h"

extern unsigned char my_mac_addr[6];

unsigned compare_guid(unsigned char *a, guid_t *b)
{
	return (a[0]==b->c[7] &&
			a[1]==b->c[6] &&
			a[2]==b->c[5] &&
			a[3]==b->c[4] &&
			a[4]==b->c[3] &&
			a[5]==b->c[2] &&
			a[6]==b->c[1] &&
			a[7]==b->c[0]);
}

int qlog2(unsigned n)
{
	int l=0;
	if (n==0) return -1;
	while ((n & 1) == 0)
	{
		n >>= 1;
		l++;
	}
	if ((n >> 1) != 0) return -1;
	return l;
}

void avb_1722_1_create_1722_1_header(	const unsigned char* dest_addr,
										int subtype,
										int message_type,
										unsigned char valid_time_status,
										unsigned data_len,
										ethernet_hdr_t *hdr)
{
	avb_1722_1_packet_header_t *pkt = (avb_1722_1_packet_header_t *) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

	for (int i=0;i<6;i++)
	{
		hdr->dest_addr[i] = dest_addr[i];
		hdr->src_addr[i] = my_mac_addr[i];
	}

	hdr->ethertype[0] = AVB_ETYPE >> 8;
	hdr->ethertype[1] = AVB_ETYPE & 0xff;

	SET_1722_1_CD_FLAG(pkt, 1);
	SET_1722_1_SUBTYPE(pkt, subtype);
	SET_1722_1_SV(pkt, 0);
	SET_1722_1_AVB_VERSION(pkt, 0);
	SET_1722_1_MSG_TYPE(pkt, message_type);
	SET_1722_1_VALID_TIME(pkt, valid_time_status);
	SET_1722_1_DATALENGTH(pkt, data_len);
}
