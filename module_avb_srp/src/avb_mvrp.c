#include "avb_mvrp.h"
#include "avb.h"
#include "avb_mrp.h"
#include "avb_mrp_pdu.h"
#include "avb_mvrp_pdu.h"
#include <xccompat.h>
#include "print.h"

void *memcpy(void *dest, const void *src, int n);

struct mvrp_entry {
  int active;
  mrp_attribute_state *attr;
  int vlan;
};

static struct mvrp_entry entries[AVB_MAX_NUM_VLAN*MRP_NUM_PORTS];

void avb_mvrp_init(void)
{
  for (int j=0;j<MRP_NUM_PORTS;j++) {
    for (int i=0;i<AVB_MAX_NUM_VLAN;i++) {
      const int idx = j*AVB_MAX_NUM_VLAN+i;
      entries[idx].active = 0;
      entries[idx].attr = mrp_get_attr();
      mrp_attribute_init(entries[idx].attr, MVRP_VID_VECTOR, j, 1, &entries[idx].vlan);
    }
  }
}

int avb_join_vlan(int vlan, int port_num)
{
  int found = -1;

  for (int i=0;i<AVB_MAX_NUM_VLAN;i++)
    if (entries[i].active && (entries[i].vlan == vlan) && (entries[i].attr->port_num == port_num))
      found = i;

  if (found == -1)
    for (int i=0;i<AVB_MAX_NUM_VLAN;i++)
      if (!entries[i].active && (entries[i].attr->port_num == port_num)) {
        found = i;
        break;
      }

  if (found == -1)
    for (int i=0;i<AVB_MAX_NUM_VLAN;i++)
      if (entries[i].active && mrp_is_observer(entries[i].attr) && (entries[i].attr->port_num == port_num)) {
        found = i;
        break;
      }

  if (found != -1) {
    entries[found].active = 1;
    entries[found].vlan = vlan;
    mrp_mad_begin(entries[found].attr);
    mrp_mad_join(entries[found].attr, 1);
    return 1;
  }

  return 0;
}

void avb_leave_vlan(int vlan)
{
  int found = -1;
  for (int i=0;i<AVB_MAX_NUM_VLAN;i++)
    if (entries[i].active && (entries[i].vlan == vlan))
      found = i;

  if (found != -1) {
    mrp_mad_leave(entries[found].attr);
  }
}

int avb_mvrp_merge_message(char *buf,
                          mrp_attribute_state *st,
                          int vector)
{
   mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr =
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));
  int merge = 0;
  int num_values;
  if (mrp_hdr->AttributeType != AVB_MVRP_VID_VECTOR_ATTRIBUTE_TYPE)
    return 0;

  num_values = hdr->NumberOfValuesLow;

  if (num_values == 0)
    merge = 1;

  if (merge) {
    mvrp_vid_vector_first_value *first_value =
      (mvrp_vid_vector_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));
    int *vlan = (int*) st->attribute_info;

    first_value->vlan[0] = (*vlan >> 8) & 0xff;
    first_value->vlan[1] = (*vlan) & 0xff;

    mrp_encode_three_packed_event(buf, vector, st->attribute_type);

    hdr->NumberOfValuesLow = num_values+1;

  }

  return merge;
}



int avb_mvrp_match_vid_vector(mrp_attribute_state *attr,
                   char *fv,
                   int i)
{

  int vlan;
  int *my_vlan = (int*) attr->attribute_info;
  mvrp_vid_vector_first_value *first_value =  (mvrp_vid_vector_first_value *) fv;

  vlan = (first_value->vlan[0] << 8) + first_value->vlan[1] + i;

  return (vlan == *my_vlan);
}

void avb_mvrp_vid_vector_join_ind(mrp_attribute_state *attr, int new)
{
}

void avb_mvrp_vid_vector_leave_ind(mrp_attribute_state *attr)
{
}
