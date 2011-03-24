#ifndef __avb_internal_h__
#define __avb_internal_h__

unsigned avb_control_get_mac_tx(void);
unsigned avb_control_get_c_ptp(void);

#ifndef __XC__
char *avb_control_get_my_mac_addr(void);
#endif

#endif
