#include <xccompat.h>
#include <print.h>
#include "simple_printf.h"
#include "avb.h"
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_1722_maap.h"
#include "avb_1722_maap_protocol.h"
#include "avb_control_types.h"

void __attribute__((weak)) avb_app_on_source_address_reserved(int offset, unsigned char mac_addr[6])
{
  // Do some debug print
  simple_printf("MAAP reserved Talker stream #%d address: %x:%x:%x:%x:%x:%x\n", offset,
                            mac_addr[0],
                            mac_addr[1],
                            mac_addr[2],
                            mac_addr[3],
                            mac_addr[4],
                            mac_addr[5]);

  set_avb_source_dest(offset, mac_addr, 6);
  set_avb_source_state(offset, AVB_SOURCE_STATE_POTENTIAL);
}