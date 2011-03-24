#include <platform.h>
#define BUTTON_TIMEOUT_VALUE 1000000


on stdcore[0]: buffered in port:4 buttons1_4 = XS1_PORT_4F;
on stdcore[0]: buffered in port:1 button5 = XS1_PORT_1J;

static  int buttons_enabled=1;
static  int button1_4value=0;
static  int button5value=0;
static  unsigned int button_timeout=0;


int poll_xdk_buttons()
{
  int res = 0;
  timer poll_tmr;
  unsigned poll_timeout;
  timer tmr;

  poll_tmr :> poll_timeout;
  poll_timeout += 1000;
  select 
    {      
    case buttons1_4 when pinsneq(button1_4value) :> button1_4value:
      {        
        if (buttons_enabled) {
          int val = button1_4value ^ 0xf;
          if (val == 1) {
            res = 1;
            tmr :> button_timeout;       
            button_timeout+=BUTTON_TIMEOUT_VALUE;
            buttons_enabled = 0;
          }
          else if (val == 2) {
            res = 2;
            tmr :> button_timeout;       
            button_timeout+=BUTTON_TIMEOUT_VALUE;
            buttons_enabled = 0;
          }
          else if (val == 4) {
            res = 3;
            tmr :> button_timeout;       
            button_timeout+=BUTTON_TIMEOUT_VALUE;
            buttons_enabled = 0;
          }
          else if (val == 8) {
            res = 4;
            tmr :> button_timeout;       
            button_timeout+=BUTTON_TIMEOUT_VALUE;
            buttons_enabled = 0;
          }
        }
      }          
      break;
    case button5 when pinsneq(button5value) :> button5value:
      if (buttons_enabled) {
        if (button5value == 0) {
          res = 5;
          tmr :> button_timeout;       
          button_timeout+=BUTTON_TIMEOUT_VALUE;
          buttons_enabled = 0;
        }
      }
      break;
    case !buttons_enabled => tmr when timerafter(button_timeout) :> int _:
      buttons_enabled = 1;
      break;
    default:
      break;
    }
  return res;
}
