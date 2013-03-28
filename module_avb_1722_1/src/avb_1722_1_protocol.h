#ifndef AVB_1722_1_PROTOCOL_H_
#define AVB_1722_1_PROTOCOL_H_

/**
 * The general header for a 1722 packet
 */
typedef struct avb_1722_1_packet_header_t {
  unsigned char cd_subtype;
  unsigned char sv_avb_version_msg_type;
  unsigned char valid_time_data_length_hi;
  unsigned char data_length_lo;

} avb_1722_1_packet_header_t;

#define AVB_1722_1_PACKET_BODY_POINTER_OFFSET 	1	// sizeof(avb_1722_1_packet_header_t) = 4 bytes

typedef union {
	unsigned long long l;
	unsigned char c[8];
} guid_t, stream_t, gmid_t;

#ifndef __XC__
typedef const guid_t * const const_guid_ref_t;
#else
typedef const guid_t & const_guid_ref_t;
#endif

#define DEFAULT_1722_1_CD_FLAG (1)
#define DEFAULT_1722_1_AVB_VERSION (0x0)

#define DEFAULT_1722_1_ADP_SUBTYPE 	(0x7a)
#define DEFAULT_1722_1_AECP_SUBTYPE (0x7b)
#define DEFAULT_1722_1_ACMP_SUBTYPE (0x7c)

#define GET_1722_1_CD_FLAG(pkt) 		((pkt)->cd_subtype >> 7)
#define GET_1722_1_SUBTYPE(pkt) 		((pkt)->cd_subtype & 0x7f)
#define GET_1722_1_SV(pkt) 				((pkt)->sv_avb_version_msg_type >> 7)
#define GET_1722_1_AVB_VERSION(pkt) 	(((pkt)->sv_avb_version_msg_type & 0x70) >> 4)
#define GET_1722_1_MSG_TYPE(pkt) 		((pkt)->sv_avb_version_msg_type & 0x0f)
#define GET_1722_1_VALID_TIME(pkt) 		(((pkt)->valid_time_data_length_hi & 0xf8) >> 3)
#define GET_1722_1_DATALENGTH(pkt) 		((((pkt)->valid_time_data_length_hi & 0x7) << 8) + \
										  ((pkt)->data_length_lo))

#define SET_BITS(p, lo, hi, val) \
  do { \
    *(p) = (*(p) & (~(((1<<(hi-lo+1))-1)<<lo))) | ((val) << lo);        \
  } while(0)

#define SET_1722_1_CD_FLAG(pkt, val) SET_BITS(&pkt->cd_subtype, 7, 7, val)
#define SET_1722_1_SUBTYPE(pkt, val) SET_BITS(&pkt->cd_subtype, 0, 6, val)
#define SET_1722_1_SV(pkt, val)                               \
    SET_BITS(&pkt->sv_avb_version_msg_type, 7, 7, val)

#define SET_1722_1_AVB_VERSION(pkt, val)                      \
    SET_BITS(&pkt->sv_avb_version_msg_type,4,6,val)

#define SET_1722_1_MSG_TYPE(pkt, val)                         \
    SET_BITS(&pkt->sv_avb_version_msg_type,0,3, val)

#define SET_1722_1_VALID_TIME(pkt, val)                                  \
    SET_BITS(&pkt->valid_time_data_length_hi, 3, 7, val)

#define SET_1722_1_DATALENGTH(pkt, val)               \
  do  \
   { \
     SET_BITS(&pkt->valid_time_data_length_hi, 0, 2, (val) >> 8); \
     SET_BITS(&pkt->data_length_lo, 0, 7, (val) & 0xff); \
   }while(0)

#define SET_WORD_CONST(member, data) \
	do { \
		member[0] = (((data) >> 24) & 0xff); \
		member[1] = (((data) >> 16) & 0xff); \
		member[2] = (((data) >> 8 ) & 0xff); \
		member[3] = (((data) >> 0 ) & 0xff); \
	} while(0);

#define GET_WORD(data, member) \
	do { \
		data = (member[0] << 24) + \
		       (member[1] << 16) + \
		       (member[2] << 8) + \
		       (member[3] << 0);\
	} while(0);

#define SET_LONG_WORD(member, data) \
	do { \
		member[0] = data.c[7]; \
		member[1] = data.c[6]; \
		member[2] = data.c[5]; \
		member[3] = data.c[4]; \
		member[4] = data.c[3]; \
		member[5] = data.c[2]; \
		member[6] = data.c[1]; \
		member[7] = data.c[0]; \
	} while(0);

#define GET_LONG_WORD(data, member) \
	do { \
		data.c[7] = member[0]; \
		data.c[6] = member[1]; \
		data.c[5] = member[2]; \
		data.c[4] = member[3]; \
		data.c[3] = member[4]; \
		data.c[2] = member[5]; \
		data.c[1] = member[6]; \
		data.c[0] = member[7]; \
	} while (0);

#define COMPARE_WORD(member, data) \
		((member[0] == (data & 0xffff)) && (member[1] == (data >> 16)))

#endif /* AVB_1722_1_PROTOCOL_H_ */
