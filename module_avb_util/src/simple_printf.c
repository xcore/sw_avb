#include <print.h>
#include <stdarg.h>

int simple_printf(char * fmt, ...)
{
  char * marker;
  int intArg;
  unsigned int uintArg;
  char * strArg;

  va_list args;

  va_start(args,fmt);

  marker = fmt;

  while (*fmt) {
    switch (*fmt)
      {
      case '%':
        if (fmt != marker) {
          char c = *fmt;
          *fmt = '\0';
          printstr(marker);
          *fmt = c;
        }
        fmt++;
	switch (*(fmt))
	  {
	  case 'd':
	    intArg = va_arg(args, int);
            printint(intArg);
            break;
	  case 'u':
	    uintArg = va_arg(args, int);
            printuint(uintArg);
            break;
	  case 'x':
	    uintArg = va_arg(args, int);
            printhex(uintArg);
	    break;
	  case 'c':
	    intArg = va_arg(args, int);
            printchar(intArg);
	    break;
	  case 's':
	    strArg = va_arg(args, char *);
            printstr(strArg);
	    break;
	  }
	fmt++;
	marker = fmt;
	break;
      default:
	fmt++;
      }
  }
  if (fmt != marker) {
    char c = *fmt;
    *fmt = '\0';
    printstr(marker);
    *fmt = c;
  }

  va_end(args);

  return 0;
}


