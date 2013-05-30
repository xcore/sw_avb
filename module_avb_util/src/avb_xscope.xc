#include <xs1.h>
#include "avb_xscope.h"

#if ENABLE_XSCOPE

extern void xscope_xlink_close(void);
extern void xscope_xlink_put_ch(unsigned char data);
extern void xscope_xlink_put_ct(void);
extern void xscope_xlink_start_token(void);
extern void xscope_xlink_init(void);

// Memory optimised version of xscope_register when no probes are used
void xscope_register_no_probes(void)
{
  if (get_local_tile_id() == 0)
  {
    xscope_xlink_init();
    xscope_xlink_start_token();
    xscope_xlink_put_ch(0); // datalen
    xscope_xlink_put_ch(10); // XSCOPE_INIT
    xscope_xlink_put_ct();

    xscope_xlink_put_ch(0);
    xscope_xlink_put_ch(18); // XSCOPE_REGISTER_DONE
    xscope_xlink_put_ct();
    xscope_xlink_close();
  }
}
#endif