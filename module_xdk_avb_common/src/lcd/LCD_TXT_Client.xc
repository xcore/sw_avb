
/**
 * ModuleName LCD Client Component for text commands.
 * @Author Ross Owen
 * @Date 15/05/2008
 * @Version 1.0
 * @Description: Simple LCD Client Component functions.
 * @Description:
 *
 *
 **/

#include <LCD_TXT_Client.h>
#include <LCD_Comp_Def.h>

/** Itoa
  * @brief unsigned to hex string
  * @return void
  * @param char s destination char array
  * @param unsigned x
  **/
void Itoa(char s[11], unsigned x)
{
  int i,y;

  //  s[0] = '0';a
  //  s[1] = 'x';

  for (i = 0; i < 8; i += 1)
  {
    y = (x >> ((7 - i) << 2)) & 0xF;
    if (y >= 10)
      s[i] = 'A' + (y - 10);
    else
      s[i] = '0' + y;
  }
  s[8] = '\0';
}



/** @brief Put hex value on screen
  * @param c  Chanend to LCD
  * @param x value to print
  * @param colour Colour to print text
  **/
void LCD_TXT_PutHex( chanend c, unsigned x, unsigned colour)
{
  char s[11];
  Itoa(s, x);
  LCD_TXT_PutStringColour(c, s, colour);
}


/** @brief Put string into char buffer
  * @param 
  **/
void LCD_TXT_PutStringColour( chanend c, char str[], unsigned int colour)
{
  int i = 0;

  c <:  LCD_CMD_PUTSTR_COL;
  c <:  colour;

  while (str[i] != '\0')
  {
    c <:  (unsigned) str[i];
    i++;
  }

  c <:  (unsigned) '\0';
  
}


void LCD_TXT_PutStringColourScroll( chanend c, char str[], unsigned int colour)
{
  int i = 0;

  c <:  LCD_CMD_PUTSTR_SCROLL;
  c <:  colour;

  while (str[i] != '\0')
  {
    c <:  (unsigned) str[i];
    i++;
  }

  c <:  (unsigned) '\0';
  
}


/** @brief Kills the text part of the LCD server
  * @param c  Chanend to LCD server
  **/
void LCD_TXT_KillServer( chanend c)
{
  c <:  LCD_CMD_KILL;  	// Send kill command
  //outct(c, XS1_CT_END);		// Shutdown channel
}


/** @brief Clear a line in the char buffer. NOTE: putChar will start from this cleared line
  * @param c  Chanend to LCD server
  * @param lineNum Number of line to clear
  **/
void LCD_TXT_ClrLine( chanend c, unsigned int lineNum)
{
  c <:  LCD_CMD_CLRLINE;
  c <:  lineNum;
}


/** @brief Write char to specific position in char buffer
  * @param ch Character to add
  * @param pos Postition in character buffer to add
  **/
void LCD_TXT_SetChar( chanend c, unsigned char ch, int pos)
{
  c <:  LCD_CMD_SETCHAR;
  c <:  (unsigned)ch;
  c  <:  pos;
}


/** @brief Client put char
  * @param c  chanend to LCD server
  * @param ch char to put
  * @param colour character colour
  **/
void LCD_TXT_PutChar( chanend c, unsigned char ch, unsigned colour)
{
  c <: LCD_CMD_PUTCHAR_COL;
  c <:  (unsigned) ch;
  c <:  colour; 
}

void LCD_TXT_PutCharScroll( chanend c, unsigned char ch, unsigned colour)
{
  c <: LCD_CMD_PUTCHAR_SCROLL;
  c <:  (unsigned) ch;
  c <:  colour; 
}


/** @brief Clear all characters
  * @param c  Chanend to LCD server
  **/
void LCD_TXT_ClrScreen( chanend c)
{
  c <:  LCD_CMD_CLRCHARS; 
}

void LCD_TXT_ScrollUp( chanend c)
{
  c <:  LCD_CMD_SCROLL_UP;
}

/* TODO
void LCD_TXT_SetTextBlendMode( chanend c, unsigned blendMode)
{
  c <:  LCD_CMD_TXT_BMODE;
  c <:  blendMode;
}

void LCD_TXT_SetTextBlendAlpha( chanend c, unsigned alpha)
{
  c <:  LCD_CMD_TXT_ALPHA;
  c <:  alpha;
}
*/
