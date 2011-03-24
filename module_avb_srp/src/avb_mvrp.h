#ifndef __AVB_MVRP_H__
#define __AVB_MVRP_H__
#include "avb_conf.h"

#ifndef AVB_MAX_NUM_VLAN
#define AVB_MAX_NUM_VLAN 4
#endif

#define AVB_MVRP_ETHERTYPE (0x88f5) 

#define AVB_MVRP_MACADDR { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x21 }
//#define AVB_MVRP_MACADDR { 0x01, 0x80, 0xc2, 0x00, 0x00, 0xe }

/** Join a VLAN.
 *
 *  This function "joins" a virtual lan that is dynamic managed by
 *  the MMRP protocol of the 802.1aj/802.1Qat standard.
 *
 *  The application can join up to ``AVB_MAX_NUM_VLAN`` vlans. This
 *  define defaults to 4 and can be changed in ``avb_conf.h``.
 *
 *  \param vlan_id            the id of the vlan to join
 *  \returns                  non-zero if sucessful, non-zero otherwise.
 *
 **/
int avb_join_vlan(int vlan_id);

/** Leave a VLAN
 *
 *  This function "leaves" a virtual lan that is dynamic managed by
 *  the MMRP protocol of the 802.1aj/802.1Qat standard. After this 
 *  call the application will no longer receive traffic on this VLAN.
 * 
 *  \param vlan_id           the id of the vlan to leave.
 *  
 */
void avb_leave_vlan(int vlan_id);

void avb_mvrp_init(void);


#ifndef __XC__
void avb_mvrp_recv_leave_all(int attribute_type);
void avb_mvrp_process(char *buf, int num);

int avb_mrp_mvrp_tx(int vector,
                    int attribute_type,
                    void *attribute_info,
                    char *buf);

int avb_mvrp_merge_message(char *buf,
                          mrp_attribute_state *st,
                           int vector);

int avb_mvrp_match(mrp_attribute_state *attr,
                   char *msg,
                   int i);

#endif

void avb_mvrp_periodic(void);

void avb_mrp_mvrp_periodic_event();

#endif
