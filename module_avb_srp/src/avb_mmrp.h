#ifndef __AVB_MMRP_H__
#define __AVB_MMRP_H__
#include "avb_conf.h"
#include "avb_control_types.h"

#define AVB_MMRP_ETHERTYPE (0x88f6) 

#ifndef AVB_MAX_MMRP_GROUPS
#define AVB_MAX_MMRP_GROUPS (AVB_NUM_SINKS*2)
#endif

#define AVB_MMRP_MACADDR { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x20 }

void avb_mmrp_init(void);

int avb_join_multicast_group(unsigned char addr[6]);

void avb_leave_multicast_group(unsigned char addr[6]);



#ifndef __XC__
void avb_mmrp_recv_leave_all(int attribute_type);
void avb_mmrp_process(char *buf, int num);

int avb_mrp_mmrp_tx(int vector,
                    int attribute_type,
                    void *attribute_info,
                    char *buf);

int avb_mmrp_merge_message(char *buf,
                          mrp_attribute_state *st,
                          int vector);

int avb_mmrp_match(mrp_attribute_state *attr,
                   char *msg,
                   int i);
#endif

void avb_mmrp_periodic(void);

void avb_mrp_mmrp_periodic_event();

#endif
