
#include "LCD_Comp_Def.h"
#include "char_buffer.h"
//#include "LCD_ServerActive.h"

extern unsigned fgColour;


/** LCD_SvrSetParam()
  * @return void
  * @brief Generic set server parameter
  * @param param command value that indicates which param to set
  * @param c  chanend to receive param val from and send resp
  */
static void LCD_SvrSetParam( chanend c, unsigned param)
{
  switch(param)
  {
    case LCD_CMD_SETCOL_FG:		// Update default text colour
      c :> fgColour;
      c <: LCD_RESP_ACK;		// Send ACK resp
      break;

    default:
      c <: LCD_RESP_NACK;
      break;
  }
}


static void LCD_SvrSetChar( chanend c)
{
  unsigned ch;
  int pos;

  c :> ch;
  c :> pos;

  if(pos < LCD_CHAR_BUFF_SIZE)
  {
    //pos = LCD_SvrLand2Port_char(pos); //TODO
    setChar(pos, (char)ch, fgColour);  // Write to char buffer and default colour
  }
}

static void LCD_SvrPutChar( chanend c)
{
  unsigned ch;

   c :> ch;
  
  putChar((char)ch, fgColour);

  //c <: LCD_RESP_ACK;

}


static void LCD_SvrClrChars( chanend c)
{
  // Clear char buffer and reset state
  clrChars();
}

static void LCD_SvrClrLine( chanend c)
{
  unsigned int lineNum;

  c :> lineNum;

  clrCharLine(lineNum);
}

static void LCD_SvrPutCharCol( chanend c)
{
  unsigned ch;
  unsigned colour;

  c :> ch;
  c :> colour;

  putChar((char)ch, colour);

  //c <: LCD_RESP_ACK;

}


static void LCD_SvrPutCharScroll( chanend c)
{
  unsigned ch;
  unsigned colour;

 c :> ch;
 c :> colour;

  putCharScroll((char)ch, colour);

  //c <: LCD_RESP_ACK;

}

// Put string into char buffer ('\0' terminated)
//TODO break into chunks
static void LCD_SvrPutStrCol( chanend c)
{
  unsigned int colour;
  unsigned  ch = (unsigned) ' ';

  // Receive colour
 c :> colour;
 
  // Receive string
  while(1)
  {
    // Receive char
    c :> ch;

    // Check for termination char
    if(ch == (unsigned)'\0')
      break;

    putChar(ch, colour); 
  }
  // TODO RESP
}


// Put string into char buffer ('\0' terminated)
//TODO break into chunks
static void LCD_SvrPutStrScroll( chanend c)
{
  unsigned int colour;
  unsigned  ch = (unsigned) ' ';

  // Receive colour
 c :> colour;
 
  // Receive string
  while(1)
  {
    // Receive char
  c :> ch;


    // Check for termination char
    if(ch == (unsigned)'\0')
      break;

    putCharScroll(ch, colour); 
  }
  // TODO RESP
}



static void LCD_SvrScrollUp() 
{
  CharBuffer_BaseLine--;
  if (CharBuffer_BaseLine < 0)
    CharBuffer_BaseLine = LCD_CHARS_HEIGHT-1;
}


// return 0 if kill
int LCD_SvrCmdExec_text(  chanend c, unsigned cmd)
{
  switch (cmd)
  {
    case LCD_CMD_SETCHAR:
      LCD_SvrSetChar(c);
      break;

    case LCD_CMD_PUTCHAR:
      LCD_SvrPutChar(c);
      break;

    case LCD_CMD_PUTSTR_COL:
      LCD_SvrPutStrCol(c);
      break;

    case LCD_CMD_PUTSTR_SCROLL:
      LCD_SvrPutStrScroll(c);
      break;

    case LCD_CMD_SETCOL_FG:
      LCD_SvrSetParam(c, LCD_CMD_SETCOL_FG);    // Generic setParam function
      break;

    case LCD_CMD_PUTCHAR_COL:
      LCD_SvrPutCharCol(c);
      break;

    case LCD_CMD_PUTCHAR_SCROLL:
      LCD_SvrPutCharScroll(c);
      break;

    case LCD_CMD_CLRCHARS:
      LCD_SvrClrChars(c);
      break;

    case LCD_CMD_CLRLINE:
      LCD_SvrClrLine(c);
      break;

    case LCD_CMD_KILL:
      return 0;
      break;

    case LCD_CMD_SCROLL_UP:
      LCD_SvrScrollUp();
      break;

    default:
        // Bad command
        // TODO: send nack
      break;

    
  }
  return 1;
}

