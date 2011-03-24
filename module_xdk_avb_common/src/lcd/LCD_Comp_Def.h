/**
 * @ModuleName Generic LCD Common Client/Server Component definitions.
 * @Description: Generic LCD Common Client/Server Component definitions.

 *
 **/

#ifndef _LCD_COMP_DEF_H
#define _LCD_COMP_DEF_H_ 1

#include <xs1.h>

// LCD Specific
/** @def LCD_WIDTH_PX
  LCD Screen with in pixels
*/
#define LCD_WIDTH_PX          240
#define LCD_HEIGHT_PX         320

// Supported commands
#define LCD_CMD_SETMODE       0x1
#define LCD_CMD_STARTFRAME    0x2
#define LCD_CMD_SETCHAR       0x3 	// Write char to char buffer
#define LCD_CMD_CLRCHARS      0x4    	// Clear char buffer
#define LCD_CMD_PUTCHAR       0x5
#define LCD_CMD_SETCOL_FG     0x6
#define LCD_CMD_PUTCHAR_COL   0x7
#define LCD_CMD_STARTLINE     0x8
#define LCD_CMD_UP_FR_INDEX   0x9   	// Update frameIndex
#define LCD_CMD_KILL          0xa   	// Kill server and accociated threads
#define LCD_CMD_CLRLINE       0xb
#define LCD_CMD_PUTSTR_COL    0xc	// Put a string into char buff (colour)
#define LCD_CMD_TXT_BMODE        0xd
#define LCD_CMD_TXT_ALPHA     0xe
#define LCD_CMD_SCROLL_UP     0xf
#define LCD_CMD_PUTSTR_SCROLL 0x10
#define LCD_CMD_PUTCHAR_SCROLL 0x11

#define LCD_TXT_BMODE_SIMPLE  0x1
#define LCD_TXT_BMODE_ALPHA   0x2
#define LCD_TXT_BMODE_ADDI    0x3

// Supported resposes
#define LCD_RESP_ACK          0x80000001
#define LCD_RESP_NACK         0x80000002

// | BLUE | GREEN | RED |

// Some useful colour defines (RGB666)
#define LCD_WHITE             0x3ffff
#define LCD_BLACK             0x00000
#define LCD_RED               0x0003f
#define LCD_GREEN             0x0FC00
#define LCD_BLUE              0x3f000
#define LCD_YELLOW            0x0FFFF
#define LCD_TEAL              0x3FFC0
#define LCD_GREY0             0x3EFBE
#define LCD_GREY1             0x3cf3c
#define LCD_GREY2             0x38e38

// RGB (BGR!)565 colour defines
#define LCD_565_RED           0x001F
#define LCD_565_BLUE	      0xf800
#define LCD_565_GREEN	      0x07e0
#define LCD_565_BLACK	      0x0000
#define LCD_565_TEAL	      0xFFE0
#define LCD_565_YELLOW	      0x07ff
#define LCD_565_WHITE	      0xffff
#define LCD_565_GREY0	      0xe79c
#define LCD_565_GREY1	      0xc718
#define LCD_565_GREY2	      0x8410
#define LCD_565_GREY3	      0x2104

// Default foreground colour
#define LCD_DEFAULT_FG 	      LCD_WHITE

// SRAM buffer size related
#define LCD_BYTES_PER_PIXEL   2     // must be 2 or 4
#define LCD_FRAME_PX          (LCD_HEIGHT_PX * LCD_WIDTH_PX)
#define LCD_FRAME_PX_2	      (LCD_FRAME_PX >> 1)
#define LCD_SRAM_FRAME_SIZE   (LCD_FRAME_PX * LCD_BYTES_PER_PIXEL)
#define LCD_SRAM_FRAME_COUNT  4     // Number of frames

#define LCD_CHARS_WIDTH       40    // Number of chars in a line
#define LCD_CHARS_HEIGHT      18    // Number of chars in a column

// DO NOT MODIFY UNLESS UPDATED CHARSET.XC
#define LCD_CHAR_WIDTH_PIX    8	   // Width of a char in pixels
#define LCD_CHAR_HEIGHT_PIX   13    // Height of a char in pixels
#define LCD_CHAR_NUM          96    // Number of different characters
#define LCD_CHAR_BUFF_SIZE    LCD_CHARS_WIDTH * LCD_CHARS_HEIGHT
#define LCD_CHAR_NUM__CHAR_HEIGHT_PX (LCD_CHAR_NUM * LCD_CHAR_HEIGHT_PIX) 

#define CHAR_BORDER           LCD_CHARS_HEIGHT * LCD_CHAR_HEIGHT_PIX

// Size of sram fifo
#define LCD_SRAM_FIFO_SIZE 6144

#define TO_565(x)  (((x & 0x3f000)>>2)  | ((x & 0xfc0)>>1) | ((x & 0x3f) >> 1))

#endif /* _LCD_COMP_DEF_H */

