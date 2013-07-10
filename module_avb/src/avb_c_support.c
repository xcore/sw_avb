#include "avb_c_support.h"
#include "nettypes.h"

void avb_get_packet_overview(unsigned int buf[], int *etype, int *eth_hdr_size, unsigned char src_addr[6]) {
  struct ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *) &buf[0];

  int has_qtag = ethernet_hdr->ethertype[1]==0x18;
  *eth_hdr_size = has_qtag ? 18 : 14;

  if (has_qtag) {
    struct tagged_ethernet_hdr_t *unsafe tagged_ethernet_hdr = (tagged_ethernet_hdr_t *unsafe) &buf[0];
    *etype = (int)(tagged_ethernet_hdr->ethertype[0] << 8) + (int)(tagged_ethernet_hdr->ethertype[1]);
  }
  else {
    *etype = (int)(ethernet_hdr->ethertype[0] << 8) + (int)(ethernet_hdr->ethertype[1]);
  }

  src_addr = ethernet_hdr->src_addr;
}