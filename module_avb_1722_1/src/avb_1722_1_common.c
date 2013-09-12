#include <print.h>
#include <xclib.h>
#include "avb_1722_common.h"
#include "avb_1722_1_common.h"

extern unsigned char my_mac_addr[6];

void print_guid_ln(const_guid_ref_t g)
{
    for (int i=7; i >= 0; i--)
    {
        printhex(g->c[i]);
        if (i == 0)
            printchar('\n');
        else
        printchar(':'); 
    }
}

void print_mac_ln(unsigned char c[6])
{
    for (int i=0; i<6; i++)
    {
        printhex(c[i]);
        if (i == 5)
            printchar('\n');
        else
        printchar(':'); 
    }
}

unsigned compare_guid(unsigned char a[6], const_guid_ref_t b)
{
    for (int i=0; i < 8; i++)
    {
        if (a[i] != b->c[7-i]) return 0;
    }
    return 1;
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

void avb_1722_1_create_1722_1_header(   const unsigned char* dest_addr,
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

    hdr->ethertype[0] = AVB_1722_ETHERTYPE >> 8;
    hdr->ethertype[1] = AVB_1722_ETHERTYPE & 0xff;

    SET_1722_1_CD_FLAG(pkt, 1);
    SET_1722_1_SUBTYPE(pkt, subtype);
    SET_1722_1_SV(pkt, 0);
    SET_1722_1_AVB_VERSION(pkt, 0);
    SET_1722_1_MSG_TYPE(pkt, message_type);
    SET_1722_1_VALID_TIME(pkt, valid_time_status);
    SET_1722_1_DATALENGTH(pkt, data_len);
}
