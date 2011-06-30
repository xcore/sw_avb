#ifndef __avb_mrp_pdu_h__
#define __avb_mrp_pdu_h__

typedef struct {
  unsigned char dest_addr[6];
  unsigned char src_addr[6];
  unsigned char ethertype[2];
} mrp_ethernet_hdr;


typedef struct {
  unsigned char ProtocolVersion;
} mrp_header;

typedef struct {
  unsigned char AttributeType;
  unsigned char AttributeLength;
  unsigned char AttributeListLength[2];
} mrp_msg_header;

typedef struct {
  unsigned char LeaveAllEventNumberOfValuesHigh;
  unsigned char NumberOfValuesLow;
} mrp_vector_header;

typedef struct {
  unsigned char EndMark[2];
} mrp_footer; 

typedef struct {
  unsigned char EndMark[2];
} mrp_msg_footer; 

#endif //__avb_mrp_pdu_h__
