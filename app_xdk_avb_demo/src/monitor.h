#ifndef __monitor_h__
#define __monitor_h__
#include <xccompat.h>

void register_monitor_ctl(chanend c);

int getset_device_monitor_source(int set, REFERENCE_PARAM(int, a0));

inline int set_device_monitor_source(int a0)
{return getset_device_monitor_source(1, REFERENCE_TO a0);}

inline int get_device_monitor_source(REFERENCE_PARAM(int, a0))
 {return getset_device_monitor_source(0, a0);}

#endif
