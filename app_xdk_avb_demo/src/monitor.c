#include <xccompat.h>
#include "c_io.h"
static chanend monitor_ctl;

static int monitor_source = 0;

void register_monitor_ctl(chanend c)
 {
  monitor_ctl = c;
}

int getset_device_monitor_source(int set, int *a0) 
{
  if (set && *a0 >= 0 && *a0 < 4) {
    monitor_source = *a0;
    xc_abi_outuint(monitor_ctl, monitor_source);
  }
  *a0 = monitor_source;
  return 1;
}

