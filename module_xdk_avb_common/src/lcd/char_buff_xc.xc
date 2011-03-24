
#include "LCD_Comp_Def.h"

extern int CharBuffer_BaseLine;


// Character buffer
unsigned char charBuff[LCD_CHARS_WIDTH * LCD_CHARS_HEIGHT];

// Character colour buffer
unsigned charColBuff[LCD_CHARS_WIDTH * LCD_CHARS_HEIGHT];

// Stores length of line (speeds up clrline etc)
int charLineSizeBuff[LCD_CHARS_HEIGHT];

int charLineSpan[LCD_CHARS_HEIGHT];

int putCharX = LCD_CHARS_HEIGHT - 1; // Current char row. Start at highest index since drawing bottom->top

int charLineCount = 0; 

unsigned putCharYCount = 0;

/** @brief This draw a character to LCD on specified position.
 **/
void setChar(int index, unsigned char c, unsigned colour)
{
  // TODO: translate positon
  charBuff[index] = c;
  charColBuff[index] = colour;
}

/* Clears character buffer (by reseting line lengths and counters)*/
void clrChars()
{
  int i;
  CharBuffer_BaseLine = 0; 
  // Reset all line lengths to 0 (faster than setting all char buff to space!)
  // Reset span flags
  for(i = 0; i < LCD_CHARS_HEIGHT; i++)
  {
    charLineSizeBuff[i] = 0;
    charLineSpan[i] = 0;
  }

  // Reset Counters
  putCharYCount = 0;
  putCharX = LCD_CHARS_HEIGHT - 1;
  charLineCount = 0;
}

/** Clear a line in the char buffer
 */
void clrCharLine(unsigned int n)
{ 
  charLineSizeBuff[n] = 0;
  charLineSpan[n] = 0;

  // Reset putChar state to current line.
  putCharX = LCD_CHARS_HEIGHT -n - 1;
  putCharYCount = 0;
  charLineCount = n;
}

/** @brief Puts a char into the char buffer, deals with newline etc
  * @param ch character to put
  * @param colour char character
  **/ 
void putChar(unsigned char ch, unsigned colour)
{
  int tmp;

  if (ch == '\n')
  {
    putCharX--;
    putCharYCount = 0;
    charLineCount++;
    if(charLineCount == LCD_CHARS_HEIGHT)
        charLineCount = 0;
    charLineSizeBuff[charLineCount] = 0; // Reset line length
     
    // If current line spans to next line clear this line also etc etc
    tmp = charLineCount;
    while(charLineSpan[tmp])
    {
      // Clear span flag
      charLineSpan[tmp] =0;

      tmp++;
      if (tmp == LCD_CHARS_HEIGHT)
        tmp = 0;
      charLineSizeBuff[tmp] = 0; 
    }
  }
  else
  {
    setChar(putCharX+putCharYCount, ch, colour);
    charLineSizeBuff[charLineCount]++;
    putCharYCount += LCD_CHARS_HEIGHT; // + no chars top to bottom

    // Check to seen if reached end of line before used \n
    if(putCharYCount == LCD_CHAR_BUFF_SIZE )
    {
      putCharYCount = 0;
      putCharX--;

      // Set span flag for line
      charLineSpan[charLineCount] = 1;

      charLineCount++;
      if(charLineCount == LCD_CHARS_HEIGHT)
        charLineCount = 0;
      charLineSizeBuff[charLineCount] = 0; // Reset line length

      // If current line spans to next line clear this line also etc etc
      tmp = charLineCount;
      while(charLineSpan[tmp])
      {
	     // Clear span flag
        charLineSpan[tmp] =0;
        tmp++;
        if (tmp == LCD_CHARS_HEIGHT)
          tmp = 0;
        charLineSizeBuff[tmp] = 0; 
      }
    }
  }

  if(putCharX == -1)
  {
    putCharX = LCD_CHARS_HEIGHT -1;
  }
}



/** @brief Puts a char into the char buffer, deals with newline etc
  * @param ch character to put
  * @param colour char character
  **/ 
void putCharScroll(unsigned char ch, unsigned colour)
{
  int tmp;

  if (ch == '\n')
  {
    putCharX--;
    putCharYCount = 0;
    charLineCount++;
    if(charLineCount == LCD_CHARS_HEIGHT)
        charLineCount = 0;
    charLineSizeBuff[charLineCount] = 0; // Reset line length
     
    // If current line spans to next line clear this line also etc etc
    tmp = charLineCount;
    while(charLineSpan[tmp])
    {
      // Clear span flag
      charLineSpan[tmp] =0;

      tmp++;
      if (tmp == LCD_CHARS_HEIGHT)
        tmp = 0;
      charLineSizeBuff[tmp] = 0; 
    }
    CharBuffer_BaseLine--;
    if (CharBuffer_BaseLine < 0)
      CharBuffer_BaseLine = LCD_CHARS_HEIGHT-1;
  }
  else
  {
    setChar(putCharX+putCharYCount, ch, colour);
    charLineSizeBuff[charLineCount]++;
    putCharYCount += LCD_CHARS_HEIGHT; // + no chars top to bottom

    // Check to seen if reached end of line before used \n
    if(putCharYCount == LCD_CHAR_BUFF_SIZE )
    {
      putCharYCount = 0;
      putCharX--;

      // Set span flag for line
      charLineSpan[charLineCount] = 1;

      charLineCount++;
      if(charLineCount == LCD_CHARS_HEIGHT)
        charLineCount = 0;
      charLineSizeBuff[charLineCount] = 0; // Reset line length

      CharBuffer_BaseLine--;
      if (CharBuffer_BaseLine < 0)
	CharBuffer_BaseLine = LCD_CHARS_HEIGHT-1;

      // If current line spans to next line clear this line also etc etc
      tmp = charLineCount;
      while(charLineSpan[tmp])
      {
	     // Clear span flag
        charLineSpan[tmp] =0;
        tmp++;
        if (tmp == LCD_CHARS_HEIGHT)
          tmp = 0;
        charLineSizeBuff[tmp] = 0; 
      }
    }
  }

  if(putCharX == -1)
  {
    putCharX = LCD_CHARS_HEIGHT -1;
  }
}

