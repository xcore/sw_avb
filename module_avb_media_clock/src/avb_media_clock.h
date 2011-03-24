#ifndef __media_clock_h__
#define __media_clock_h__
#include <xccompat.h>

void media_clock_set_rate(chanend media_clock_svr, int media_clock_num, int rate);
int  media_clock_get_rate(chanend media_clock_svr, int media_clock_num);

void media_clock_set_type(chanend media_clock_svr, int media_clock_num, int rate);
int  media_clock_get_type(chanend media_clock_svr, int media_clock_num);

void media_clock_set_source(chanend media_clock_svr, int media_clock_num, 
                         int a);

void media_clock_get_source(chanend media_clock_svr, int media_clock_num, 
                         REFERENCE_PARAM(int,a));


void media_clock_set_state(chanend media_clock_svr, int media_clock_num, int state);
int  media_clock_get_state(chanend media_clock_svr, int media_clock_num);


void media_clock_register(chanend media_clock_svr, int clk_ctl, int clk_num);
#endif 
