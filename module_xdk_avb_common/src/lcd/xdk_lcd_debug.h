#ifndef __xlog_lcd_debug_h__
#define __xlog_lcd_debug_h__


void xdk_lcd_init_debug_screen(chanend txt_svr);


void xdk_xlog_to_lcd(chanend debug_ch, chanend txt_svr, int display_result);

#endif // __xlog_lcd_debug_h__
