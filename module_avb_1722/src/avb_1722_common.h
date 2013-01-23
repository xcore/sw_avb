/**
 * \file avb_1722_common.h
 * \brief AVB 1722 common definitions
 */


#ifndef _AVB_COMMON_H_ 
#define _AVB_COMMON_H_ 1


// Network to/from Host conversion
#define NTOH_U16(x) ( ((unsigned) x[0] << 8) | ((unsigned) x[1]) )
#define NTOH_U32(x) ( ((unsigned) x[0] << 24) | ((unsigned) x[1] << 16) | ((unsigned) x[2] << 8) | ((unsigned) x[3]) )
#define NTOH_U32i(x,i) ( ((unsigned) x[i] << 24) | ((unsigned) x[i+1] << 16) | ((unsigned) x[i+2] << 8) | ((unsigned) x[i+3]) )


#define HTON_U16(x, v) do { x[0] = (v >> 8) & 0xFF; \
                            x[1] = (v & 0xFF); } while (0)
#define HTON_U32(x, v) do { x[0] = (unsigned char) (v >> 24);    \
                            x[1] = (unsigned char) (v >> 16); \
                            x[2] = (unsigned char) (v >> 8) ; \
                            x[3] = (unsigned char) (v); } while (0)

#define HTON_U32i(x, i, v) do { x[i+0] = (v >> 24);       \
                                x[i+1] = (v >> 16);                          \
                                x[i+2] = (v >> 8) ; \
                                x[i+3] = (v); } while (0)

// Frame Header size in bytes definations.                             
#define AVB_ETHERNET_HDR_SIZE    (18)
#define AVB_TP_HDR_SIZE          (24)

// number of bytes in MAC address
#define MAC_ADRS_BYTE_COUNT      (6)
                            
                            
// AVB frame header
typedef struct
{
  unsigned char DA[MAC_ADRS_BYTE_COUNT];    // dest MAC addr
  unsigned char SA[MAC_ADRS_BYTE_COUNT];    // src MAC addr
  unsigned char TPID[2];  // 802.1P/Q = 8100
  unsigned char flagVID[2];  // contains PCP (priority code point), bits 0,1,2.
                          //          CFI (canonical format indicator), bit 3.
                          // VID bits 4-15
  unsigned char etype[2];    
}AVB_Frame_t;


// Macros to access the AVBTP transport layer.
// Usage: 
// 1. "x" in following macros are pointer to valid AVBTP_FrameHeader.
// 2. Return the value of the item in HOST byte order.
#define AVBTP_PCP(x)          (x->flagVID[0] >> 5)
#define AVBTP_CFI(x)          ((x->flagVID[0] & 0x10) >> 4)
#define AVBTP_VID(x)          (((x->flagVID[0] & 0xF) << 8) | x->flagVID[1])
#define AVBTP_TPID(x)         ((x->TPID[0] << 8) | x->TPID[1])
#define AVBTP_ETYPE(x)        ((x->etype[0] << 8) | x->etype[1])

// Macros to set the AVBTP transport layer.
// Usgae:
// 1. "x" in following macros are pointer to valid AVBTP_FrameHeader.
// 2. "a" is the value in HOST byte order to set.
#define SET_AVBTP_PCP(x, a)   do{ x->flagVID[0] |= (a << 5); } while (0)
#define SET_AVBTP_CFI(x, a)   do{ x->flagVID[0] |= (a & 0x1) << 4; } while (0)
#define SET_AVBTP_VID(x, a)   do{ x->flagVID[0] |= (a & 0xF00) >> 8; \
                                  x->flagVID[1] = (a & 0xFF); } while (0)
#define SET_AVBTP_TPID(x, a)  do{ x->TPID[0] = (a >> 8); \
                                  x->TPID[1] = a & 0xFF; } while (0)
#define SET_AVBTP_ETYPE(x, a) do{ x->etype[0] = (a >> 8); \
                                  x->etype[1] = a & 0xFF; } while (0)
                                                                                   
// AVB common stream data header format
typedef struct
{
  unsigned char subtype;          // bit 0   : cd (control/data indicator). data (0), control(1)
                                  // bit 1-7 : subtype                                  
  unsigned char version_flags;    // bit 0   : sv. stream id field valid.
                                  // bit 1-3 : version.
                                  // bit 4   : r. Reserved.
                                  // bit 5   : lp. late presentation field valid.
                                  // bit 6   : gv. gateway info field valid
                                  // bit 7   : tv. timestamp field valid    
  unsigned char sequence_number;  //
  unsigned char gm_discontinunity_holdover; // bit 0-6 : GM Discontinunity counter.
                                            // bit 7   : h. holdover.
  unsigned char stream_id[8];     // 802.1Qat Stream ID
  unsigned char avb_timestamp[4]; // 
  unsigned char gateway_info[4];  // 
  unsigned char packet_data_length[2];  // length of data following the protocol specific packet header.
                                        // Max value 1476                                        
  unsigned char protocol_specific[2];
  //For 61883
  //unsigned char tag_channel;     // bit 0-1 : tag
  //                               // bit 2-7 : channel
  //unsigned char tcode_sy;        // bot 0-3 : tcode
  //                               // bit 4-7 : sy (application specific

} AVB_DataHeader_t;


// Macros to access the AVB Common Header.
// Usage: 
// 1. "x" in following macros are pointer to valid AVB Common Header.
// 2. Return the value of the item in HOST byte order.
#define AVBTP_CD(x)                    (x->subtype >> 7)
#define AVBTP_SUBTYPE(x)               (x->subtype & 0x7F)
#define AVBTP_SV(x)                    (x->version_flags >> 7)
#define AVBTP_VERSION(x)               ((x->version_flags >> 4) & 0x7)
#define AVBTP_LATE_PRESENTATION(x)     ((x->version_flags >> 2) & 0x1)
#define AVBTP_GV(x)                    ((x->version_flags >> 1) & 0x1)
#define AVBTP_TV(x)                    (x->version_flags & 0x1)
#define AVBTP_SEQUENCE_NUMBER(x)       (x->sequence_number)
#define AVBTP_GM_DISCONTINUNITY(x)     (x->gm_discontinunity_holdover >> 1)
#define AVBTP_HOLDOVER(x)              (x->gm_discontinunity_holdover & 1)
#define AVBTP_TIMESTAMP(x)             ((x->avb_timestamp[0] << 24) | \
                                        (x->avb_timestamp[1] << 16) | \
                                        (x->avb_timestamp[2] << 8) | \
                                        (x->avb_timestamp[3]))                                
#define AVBTP_STREAM_ID1(x)            ((x->stream_id[0] << 24) | \
                                        (x->stream_id[1] << 16) | \
                                        (x->stream_id[2] << 8) | \
                                        (x->stream_id[3]))                               
#define AVBTP_STREAM_ID0(x)            ((x->stream_id[4] << 24) | \
                                        (x->stream_id[5] << 16) | \
                                        (x->stream_id[6] << 8) | \
                                        (x->stream_id[7]))
#define AVBTP_PROTOCOL_SPECIFIC(x)      ((x->protocol_specific[0] << 8) | \
                                        (x->protocol_specific[1]))


// Macros to set the AVBTP transport layer.
// Usgae:
// 1. "x" in following macros are pointer to valid AVBTP_FrameHeader.
// 2. "a" is the value in HOST byte order to set.
#define SET_AVBTP_CD(x, a)                (x->subtype |= a << 7)
#define SET_AVBTP_SUBTYPE(x, a)           (x->subtype |= (a & 0x7F))
#define SET_AVBTP_SV(x, a)                (x->version_flags |= (a & 0x1) << 7)
#define SET_AVBTP_VERSION(x, a)           (x->version_flags |= (a & 0x7) << 4)    
#define SET_AVBTP_LATE_PRESENTATION(x, a) (x->version_flags |= (a & 0x1) << 2)
#define SET_AVBTP_GV(x, a)                (x->version_flags |= (a & 0x1) << 1)
#define SET_AVBTP_TV(x, a)                (x->version_flags = (x->version_flags & ~0x1) | (a & 0x1))
#define SET_AVBTP_SEQUENCE_NUMBER(x, a)   (x->sequence_number = (a & 0xff))
#define SET_AVBTP_GM_DISCONTINUNITY(x, a) (x->gm_discontinunity_holdover |= (a & 0x7F) << 1)
#define SET_AVBTP_HOLDOVER(x, a)          (x->gm_discontinunity_holdover |= a & 0x1)                                           
#define SET_AVBTP_TIMESTAMP(x, a)        do {x->avb_timestamp[0] = (a >> 24) & 0xFF; \
                                           x->avb_timestamp[1] = (a >> 16) & 0xFF; \
                                           x->avb_timestamp[2] = (a >> 8) & 0xFF; \
                                           x->avb_timestamp[3] = (a & 0xFF); } while (0)
#define SET_AVBTP_STREAM_ID1(x, a)       do {x->stream_id[0] = (a >> 24) & 0xFF; \
                                           x->stream_id[1] = (a >> 16) & 0xFF; \
                                           x->stream_id[2] = (a >> 8) & 0xFF; \
                                           x->stream_id[3] = (a & 0xFF); } while (0)
#define SET_AVBTP_STREAM_ID0(x, a)       do {x->stream_id[4] = ((a) >> 24) & 0xFF; \
                                           x->stream_id[5] = (a >> 16) & 0xFF; \
                                           x->stream_id[6] = (a >> 8) & 0xFF; \
                                           x->stream_id[7] = (a & 0xFF); } while (0)                                           
#define SET_AVBTP_GATEWAY_INFO(x, a)     do {x->gateway_info[0] = (a >> 24) & 0xFF; \
                                           x->gateway_info[1] = (a >> 16) & 0xFF; \
                                           x->gateway_info[2] = (a >> 8) & 0xFF; \
                                           x->gateway_info[3] = (a & 0xFF); } while (0)
#define SET_AVBTP_PACKET_DATA_LENGTH(x, a)  do {x->packet_data_length[0] = a >> 8; \
                                                x->packet_data_length[1] = a & 0xFF; } while (0)
#define SET_AVBTP_PROTOCOL_SPECIFIC(x, a)   do {x->protocol_specific[0] = a >> 8; \
                                                x->protocol_specific[1] = a & 0xFF; } while (0)
                                                                                      
// constants.
#define AVBTP_CD_DATA      (0)
#define AVBTP_CD_CONTROL   (1)
#define AVB_TPID           (0x8100)
#define AVB_DEFAULT_PCP    (0x3)
#define AVB_DEFAULT_CFI    (0x0)
#define AVB_DEFAULT_VID    (0x2)
#define AVB_1722_ETHERTYPE          (0x22f0)

// Default to 2ms delay.
#define AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS              (2000000)

// Number of transport stream packets to allow in each 61883-4 encapsulation
#define MAX_TS_PACKETS_PER_1722 4
#endif


short ntoh_16(unsigned char x[2]);
int ntoh_32(unsigned char x[4]) ;
void get_64(unsigned char g[8], unsigned char c[8]);
void set_64(unsigned char g[8], unsigned char c[8]);

void hton_16(unsigned char x[2], unsigned short v);

void hton_32(unsigned char x[4], unsigned int v);