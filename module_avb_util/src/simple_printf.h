#ifndef _simple_printf_h_
#define _simple_printf_h_

/**
 *  A very simple (low memory) version of printf.
 *  Accepts only %d, %x, %s, %u and %c with no formatting.
 */
int simple_printf(char fmt[], ...);

#endif // _simple_printf_h_
