#ifndef _LcdDefines_h_
#define _LcdDefines_h_

#include <xs1.h>

#define LCD_WIDTH_PX          240
#define LCD_HEIGHT_PX         320

// LCD Timing defines
#define T_HFP                 280 // 400   
#define T_HBP                 620 //600
#define T_WH                  150 // 150  
#define T_VBP                 20000 //20000 
#define LCD_CLKDIV            20

// LCD ports
#define LCD_PORT_HSYNC        XS1_PORT_4F    /* !! 4F on XDK !! */
#define LCD_PORT_DTMG         XS1_PORT_1B
#define LCD_PORT_DCLK         XS1_PORT_1A
#define LCD_PORT_RGB          XS1_PORT_32A

// LCD clockblock
#define LCD_CLKBLK            XS1_CLKBLK_4

// Some useful colour defines
#define LCD_WHITE             0x3ffff
#define LCD_BLACK             0x00000
#define LCD_RED               0x3f000
#define LCD_GREEN             0x0FC00
#define LCD_BLUE              0x0003F
#define LCD_TEAL              0x0FFFF
#define LCD_YELLOW            0x3FFC0



#endif      // _LcdDefines_h_
