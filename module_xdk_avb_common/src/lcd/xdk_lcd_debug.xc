#include "LCD_TXT_Client.h"
#define DEBUG_TXT_COLOUR LCD_WHITE

void xdk_lcd_init_debug_screen( chanend txt_svr)
{
  LCD_TXT_ClrScreen(txt_svr);
  LCD_TXT_ClrLine(txt_svr, 0);
  LCD_TXT_PutStringColour(txt_svr, "\n\n\n\n\n\n\n\n              DEBUG LOGGING", LCD_RED);
  LCD_TXT_PutStringColour(txt_svr,          "\n\n\n\n\n\n      ============================== \n\n\n", DEBUG_TXT_COLOUR);

  LCD_TXT_PutStringColourScroll(txt_svr,"\n",DEBUG_TXT_COLOUR);

}

#define DEBUG_CHAN_POLL_INTERVAL 1000



void xdk_xlog_to_lcd(chanend debug_ch,  chanend txt_svr, int display_result) {
  unsigned char c;
  select 
    {
    case inuchar_byref(debug_ch, c): {
      int data;
      (void) inuchar(debug_ch);
      (void) inuchar(debug_ch);
      (void) inct(debug_ch);
      debug_ch <: 1;
      debug_ch :> data;
      if (display_result)
        LCD_TXT_PutCharScroll(txt_svr,data,DEBUG_TXT_COLOUR);
      }
      break;
    default:
      break;
    }
  return;
}
