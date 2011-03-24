#include <platform.h>
#include "rgb_driver.h"
#include "rgb_server.h"

#define NUM_LEDS 8


#ifdef XDK
on stdcore[2]: out port sclk  = PORT_RGB_SCLK;
on stdcore[2]: out port sin   = PORT_RGB_SIN;
on stdcore[2]: out port csel  = PORT_RGB_CSEL;
on stdcore[2]: out port latch = PORT_RGB_LATCH;
on stdcore[2]: out port blank = PORT_RGB_BLANK;
#endif

void rgb_server(chanend c_rgb)
{
  // RGB pattern store
  unsigned  patterns[3] = {0x00, 0x00, 0x00};
  unsigned char enabled[3] = {0, 0, 0};
  int i;
  timer t;
  unsigned time;
  unsigned tmp;
  unsigned active = 1;

#ifdef XDK
  blank <: 0;
#endif  
  while (active)
  {
    t :> time;
  
    select
    {
      case t when timerafter(time+15000) :> time:
        for (i = 0; i < 400; i += 1)
        { 
#ifdef XDK
          DriveLeds(blank, csel, sclk, sin, latch, 1000, patterns);
#endif
        }

        for(i = 0; i < 3; i++)
        {
          if(enabled[i])
          {  
            patterns[i] = patterns[i] << 1 ;
            
            // Rotate
            tmp = (patterns[i] & 0x100)>>8; // Bit shifted off
            patterns[i] &= 0xfe;
            patterns[i] |= tmp;
          }
        }
  
        break;
      
      case c_rgb :> tmp:
        switch(tmp)
        {
          case RGB_CMD_ENABLE_BLUE_SPIN:
            enabled[BLUE] = 1;
            break;

          case RGB_CMD_ENABLE_GREEN_SPIN:
            enabled[GREEN] = 1;
            break;

          case RGB_CMD_ENABLE_RED_SPIN:
            enabled[RED] = 1;
            break;
          
          case RGB_CMD_UPDATE_PATTERNS:
            c_rgb :> patterns[0];
            c_rgb :> patterns[1];
            c_rgb :> patterns[2];
            break;

          case RGB_CMD_DISABLE_BLUE_SPIN:
            enabled[BLUE] = 0;
            break;

          case RGB_CMD_DISABLE_GREEN_SPIN:
            enabled[GREEN] = 0;
            break;

          case RGB_CMD_DISABLE_RED_SPIN:
            enabled[RED] = 0;
            break;
          
          case RGB_CMD_GET_BLUE_PATTERN:
            c_rgb <: patterns[BLUE];
            break;

          case RGB_CMD_GET_RED_PATTERN:
            c_rgb <: patterns[RED];
            break;

          case RGB_CMD_GET_GREEN_PATTERN:
            c_rgb <: patterns[GREEN];
            break;
  
          case RGB_CMD_KILL:
            active = 0;
            break;
        } 
      break;
    }
  }
}
