
// This deals with character buffering and interpretation of commands, putchar, etc



#include "LCD_Comp_Def.h"
#include "char_buffer.h"
//#include "LCD_ServerActive.h"
#include "charset.h"
//#include "xlog_client.h"
#include "LCD_CharCmds.xc"
#include "LCD_Defines_Internal.h"

extern unsigned fgColour;

// Character buffer
extern unsigned char charBuff[LCD_CHARS_WIDTH * LCD_CHARS_HEIGHT];

extern unsigned charColBuff[LCD_CHARS_WIDTH * LCD_CHARS_HEIGHT];

extern int charLineSizeBuff[LCD_CHARS_HEIGHT];

int CharBuffer_BaseLine = 0;

/** LCD_CharBuffer()
  * @return void
  * @brief Character buffer thread. This thread reads from the character buffer and assembles pixel data and outputs to the blender thread.
  * This thread also takes commands via channel from application to add chars etc.
  * @param c_lcd_txt channel from app (for commands.. putChar etc)
  * @param c chanend (streamed) to blender thread
  */
void lcd_text_display( chanend c_lcd_txt,chanend c_char_blender)

{
  unsigned int VCount, HCount, tmp;
  unsigned int colour ;
  int colCount = 0;
  int rowCount;
  int charcount = 0;
  int charCountSave;
  int col;
  unsigned charLineNo = 0;
  int charLineCount;
  int active = 1;
  int x,y;


  while(active)
  {
    charcount = 0;
    colCount = 0;
    charLineNo = LCD_CHARS_HEIGHT-1 - CharBuffer_BaseLine;// charLine is what line we are on (0 bottom, 19 top line);
    charLineCount = 0;  	       // How many chars outputted on current line
    col = 0;

    // TODO: Could do with a priority select here?
    select
    {
      //    case start__slave(c_char_blender):
    case chkct(c_char_blender, XS1_CT_END):
      outct(c_char_blender, XS1_CT_ACK);
      x = 0;
      y = CharBuffer_BaseLine;

      for(VCount = 0; VCount < LCD_HEIGHT_PX; VCount+=1)
      {
        rowCount = 0;

        if(colCount == 0)
          charCountSave = charcount;
 
        #pragma unsafe arrays
        for(HCount = 0; HCount < LCD_WIDTH_PX; HCount++)
        {
          // Dont go off edge of screen
          if(HCount < CHAR_BORDER && charLineSizeBuff[charLineNo] > charLineCount)
          {
	    //            tmp = charBuff[charcount];
	    tmp = charBuff[x*(LCD_CHARS_HEIGHT)+y];
            tmp -= ' ';
    
	    //            colour = charColBuff[charcount];
	    colour = charColBuff[x*(LCD_CHARS_HEIGHT)+y];
            tmp = col + (tmp *LCD_CHAR_HEIGHT_PIX) + rowCount;

            // Chars pixesl stores as 8 bit values. Make a RGB565 value out of this 
            // TODO: Change this!
            tmp = charset[tmp];
            tmp = ((tmp>>3)<< 11) | ((tmp>>2)<<5) |(tmp>>3);
            tmp &= colour;
          }
          else
	  {
	    tmp = 0;   
	  }

          // Output char pixel data to blender thread
          outuint(c_char_blender,tmp);

          rowCount++;
          if(rowCount == LCD_CHAR_HEIGHT_PIX)
          {
            rowCount = 0;
            charcount++;	
	    y++;
	    if (y > LCD_CHARS_HEIGHT-1)
	      y = 0;
	    if (charLineNo == 0) 
	      charLineNo = LCD_CHARS_HEIGHT - 1;
	    else
	      charLineNo--;

          }
        }
        col += (LCD_CHAR_NUM * LCD_CHAR_HEIGHT_PIX);

        charLineNo = LCD_CHARS_HEIGHT - 1 - CharBuffer_BaseLine; 
        colCount++;
        if (colCount == LCD_CHAR_WIDTH_PIX)
        { col = 0;
          colCount = 0;
          charLineCount++;
	  x++;
        }
        else
        {
          charcount = charCountSave;
        }
	y = CharBuffer_BaseLine;

      }
      outct(c_char_blender, XS1_CT_END);
      chkct(c_char_blender, XS1_CT_END);
      break;

    case c_lcd_txt :> tmp:		    // Take command
      active = LCD_SvrCmdExec_text(c_lcd_txt, tmp); // Returns 0 if kill
      break;
    }
  }

}
