#ifndef __avb_mrp_pdu_h__
#define __avb_mrp_pdu_h__

/** \file avb_mrp_pdu.h
 *
 * An MRP PDU (MRPDU) has a standard ethernet header, followed by an
 * MRP header that contains only the protocol version.  Following that
 * are a list of N messages and a two byte 0000 terminator.
 *
 * Each message has a header, containing the attribute type and the
 * length of each 'first value'.  In the case of SRP only, this is
 * followed by an attribute list length.  Then comes the attribute list
 * itself.  The attribute list is a list of entries followed by a
 * list terminator of two bytes 0000.
 *
 * Each attribute list entry (we're calling them 'vector' in our code)
 * there is a header containing the number of values that this entry
 * encodes (plus a 'leaveall' event notifier), then the 'first value',
 * then an array of packed data values.
 *
 * If it sounds complex then that is because it _is_ complex, and good
 * luck reading the spec, you'll need it.
 *
 * The reason why the attribute list exists is so that a sparse set of
 * generic values can be described reasonably compactly.  The 'first
 * values' are a given piece of data, for example the Stream Id which is
 * the data value for the Listener Ready attribute.  You can then say
 * that this attribute list entry is actually for 4 Stream Ids, and
 * give a packed array of data values for each of the 4 stream ids.
 *
 * You can then have another attribute list entry for another stream id
 * and have that one represent another contiguous group.   In this way
 * you can represent a sparse set reasonably compactly.  At the expense,
 * of course, of horrible complexity in the code.
 */


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
