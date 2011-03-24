#ifndef __avb_util_h__
#define __avb_util_h__

#ifndef __XC__
int avb_itoa(int n, char *buf, int base, int fill);

int avb_itoa_fixed(int n, char *buf, int base, int fill1, int fill2, int prec);

char *avb_atoi(char *buf, int *x0);
#endif


#endif // __avb_util_h__

