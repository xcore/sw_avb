#include "avb.h"
#include "avb_mmrp.h"
#include "avb_mrp.h"
#include "avb_mrp_pdu.h"
#include "avb_mmrp_pdu.h"
#include <xccompat.h>
#include "avb_internal.h"

#ifdef AVB_INCLUDE_MMRP

void *memcpy(void *dest, const void *src, int n);

struct mmrp_entry {
  int active;
  mrp_attribute_state *attr;
  unsigned char addr[6];
};

static struct mmrp_entry entries[AVB_MAX_MMRP_GROUPS];

static int addr_eq(unsigned char *x, unsigned char *y)
{
  for (int i=0;i<6;i++)
    if (x[i] != y[i])
      return 0;
  return 1;
}

void avb_mmrp_init(void)
{
  for (int i=0;i<AVB_MAX_MMRP_GROUPS;i++) {
    entries[i].active = 0;
    entries[i].attr = mrp_get_attr();
    mrp_attribute_init(entries[i].attr, MMRP_MAC_VECTOR, 0, &entries[i].addr);
  }
}

int avb_join_multicast_group(unsigned char addr[6])
{
  int found = -1;
  
  for (int i=0;i<AVB_MAX_MMRP_GROUPS;i++)
    if (entries[i].active && addr_eq(addr, entries[i].addr)) 
      found = i;

  if (found == -1) 
    for (int i=0;i<AVB_MAX_MMRP_GROUPS;i++)
      if (!entries[i].active) {
        found = i;
        break;
      }

  if (found == -1)
    for (int i=0;i<AVB_MAX_MMRP_GROUPS;i++)
      if (entries[i].active && mrp_is_observer(entries[i].attr)) {
        found = i;
        break;
      }


  if (found != -1) {
    entries[found].active = 1;
    memcpy(entries[found].addr, addr, 6);
    mrp_mad_begin(entries[found].attr);
    mrp_mad_join(entries[found].attr, 1);
    return 1;
  }

  return 0;
}

void avb_leave_multicast_group(unsigned char addr[6])
{
  int found = -1;
  for (int i=0;i<AVB_MAX_MMRP_GROUPS;i++)
    if (entries[i].active && addr_eq(addr, entries[i].addr)) 
      found = i;
  
  if (found != -1) {
    mrp_mad_leave(entries[found].attr);
  }
}

int avb_mmrp_merge_message(char *buf,
                          mrp_attribute_state *st,
                          int vector)
{
   mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr = 
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));  
  int merge = 0;
  int num_values;
  if (mrp_hdr->AttributeType != AVB_MMRP_MAC_VECTOR_ATTRIBUTE_TYPE)
    return 0;

  num_values = hdr->NumberOfValuesLow;
                           
  if (num_values == 0) 
    merge = 1;

  if (merge) {
	  mmrp_mac_vector_first_value *first_value =
      (mmrp_mac_vector_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));
    char *addr = (char *) st->attribute_info;

    for (int i=0;i<6;i++)   
      first_value->addr[i] = addr[i];
 
    mrp_encode_three_packed_event(buf, vector, st->attribute_type);    

    hdr->NumberOfValuesLow = num_values+1;

  }

  return merge;   
}



int avb_mmrp_match_mac_vector(mrp_attribute_state *attr,
                   char *fv,
                   int i)
{

  unsigned long long addr=0, my_addr=0;

  char *a = (char *) attr->attribute_info;
  mmrp_mac_vector_first_value *first_value = (mmrp_mac_vector_first_value *) fv;

  for (int i=0;i<6;i++) {
    my_addr = (my_addr << 8) + (unsigned char) a[i];
    addr = (addr << 8) + first_value->addr[i];
  }
  
  addr += i;

  return (addr == my_addr);
}

void avb_mmrp_mac_vector_join_ind(mrp_attribute_state *attr, int new)
{
}

void avb_mmrp_mac_vector_leave_ind(mrp_attribute_state *attr)
{
}

#endif


