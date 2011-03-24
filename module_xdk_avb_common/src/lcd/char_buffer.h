/*
 * @ModuleName Character buffer for LCD display.
 * @Author Ross Owen
 * @Date 13/05/2008
 * @Version 1.0
 * @Description: Character buffer for LCD display.
 *
 *
 */


#ifndef _CHAR_BUFFER_H_
#define _CHAR_BUFFER_H_ 1

#include "LCD_Comp_Def.h"

/** This draw a character to LCD on specified position.
 */
void setChar (int index, unsigned char c, unsigned colour);

/** Returns specified char from char buffer
 */
unsigned char getChar(int index);

/** Return the RGB565 pixel information for a given x, y,
 *  character buffer.
 */
int GetCharPixel(int x, int y);

/** Puts a char into the char buffer, deals with newline etc
 */
void putChar(unsigned char ch, unsigned colour);

void putCharScroll(unsigned char ch, unsigned colour);

void clrChars();

unsigned getCharColour(int index);

int getCharLineLength(int index);

void setCharLineLength(int index, int length);

void clrCharLine(unsigned int n);

void setCharLineSpan(int index, int val);

extern int CharBuffer_BaseLine;

#endif
