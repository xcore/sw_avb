
/**
 * ModuleName LCD Client text commands.
 * @Author Ross Owen
 * @Date 22/10/2008
 * @Version 1.1
 * @Description: Simple LCD Client Component functions
 *
 *
 **/

#ifndef _LCD_TXT_CLIENT_H_
#define _LCD_TXT_CLIENT_H_ 1

#include <LCD_Comp_Def.h>

// Converts unsigned to hex string and outputs on screen
void LCD_TXT_PutHex( chanend c, unsigned word, unsigned colour);

// Writes string to charbuffer '\0' terminated
void LCD_TXT_PutStringColour( chanend c, char str[], unsigned int colour);

// Write char to specific position in char buffer
void LCD_TXT_SetChar( chanend c, unsigned char ch, int pos);

// Put char (accepts colour)
void LCD_TXT_PutChar( chanend c, unsigned char ch, unsigned colour);

// Clears char buffer
void LCD_TXT_ClrScreen( chanend c);

// Clears a line in charbuffer. Note: the next putChar will start at the cleared line
void LCD_TXT_ClrLine( chanend c, unsigned int lineNum);

// Kill char buffer threads.  We kill this seperately so LCD driver can be used without char threads if desired
void LCD_TXT_KillServer( chanend c);

void LCD_TXT_ScrollUp( chanend c);

void LCD_TXT_PutStringColourScroll( chanend c, char str[], unsigned int colour);
void LCD_TXT_PutCharScroll( chanend c, unsigned char ch, unsigned colour);

//TODO
//void LCD_TXT_SetTextBlendMode(chanend c, unsigned blendMode);
//void LCD_TXT_SetTextBlendAlpha(chanend c, unsigned alpha);

#endif /* _LCD_TXT_CLIENT_H_ */


