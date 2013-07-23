#include "avb_media_clock.h"
#include "avb_media_clock_def.h"

unsafe void media_clock_register(chanend media_clock_svr, chanend *unsafe clk_ctl, int clk_num)
{
  media_clock_svr <: MEDIA_CLOCK_REGISTER;
  master {
    media_clock_svr <: clk_ctl;
    media_clock_svr <: clk_num;
  }
}

void media_clock_set_rate(chanend media_clock_svr, int media_clock_num, int rate)
{

  media_clock_svr <: MEDIA_CLOCK_SET_RATE;
  master {
    media_clock_svr <: media_clock_num;
    media_clock_svr <: rate;
  }
}

int  media_clock_get_rate(chanend media_clock_svr, int media_clock_num)
{
  int rate;
    media_clock_svr <: MEDIA_CLOCK_GET_RATE;
  master {
    media_clock_svr <: media_clock_num;
    media_clock_svr :> rate;
  }
  return rate;
}


void media_clock_set_type(chanend media_clock_svr, int media_clock_num, int type)
{
  media_clock_svr <: MEDIA_CLOCK_SET_TYPE;
  master {
    media_clock_svr <: media_clock_num;
    media_clock_svr <: type;
  }
}

int  media_clock_get_type(chanend media_clock_svr, int media_clock_num)
{
  int type;
  media_clock_svr <: MEDIA_CLOCK_GET_TYPE;
  master {
    media_clock_svr <: media_clock_num;
    media_clock_svr :> type;
  }
  return type;
}


void media_clock_set_source(chanend media_clock_svr, int media_clock_num, 
                         int x)
{  
  media_clock_svr <: MEDIA_CLOCK_SET_SOURCE;
  master {
    media_clock_svr <: media_clock_num;
    media_clock_svr <: x;
  }
}


void media_clock_get_source(chanend media_clock_svr, int media_clock_num, 
                         int &x)
{
  media_clock_svr <: MEDIA_CLOCK_GET_SOURCE;
  master {
    media_clock_svr <: media_clock_num;
    int source;
    media_clock_svr :> source;
    x = source;
  }
}


void media_clock_set_state(chanend media_clock_svr, int media_clock_num, int state)
{

  media_clock_svr <: MEDIA_CLOCK_SET_STATE;
  master {
    media_clock_svr <: media_clock_num;
    media_clock_svr <: state;
  }
}

int  media_clock_get_state(chanend media_clock_svr, int media_clock_num)
{
  int state;
    media_clock_svr <: MEDIA_CLOCK_GET_STATE;
  master {
    media_clock_svr <: media_clock_num;
    media_clock_svr :> state;
  }
  return state;
}
