static void reverse_array(char buf[], unsigned size)
{
  int begin = 0;
  int end = size - 1;
  int tmp;
  for (;begin < end;begin++,end--) {
    tmp = buf[begin];
    buf[begin] = buf[end];
    buf[end] = tmp;
  }
}


/* There is at most one digit per bit. So the maximum buffer needed is equal to
 * the number of bits in an int.
 */
int avb_itoa(int n, char *buf, int base, int fill)
{ static const char digits[] = "0123456789ABCDEF";
  int i = 0;
  while (n > 0) {
    int next = n / base;
    int cur  = n % base;
    buf[i] = digits[cur];
    i += 1;
    fill--;
    n = next;
  }
  for (;fill > 0;fill--) {
    buf[i] = '0';
    i++;
  }
  reverse_array(buf, i);
  return i;
}

int avb_itoa_fixed(int n, char *buf, int base, int fill1, int fill2, int prec)
{
  long long x;
  char *buf0 = buf;
  if (n < 0) {
    buf[0] = '-';
    x = -n;
  }
  else {
    buf[0] = ' ';
    x = n;
  }
  buf++;
  buf += avb_itoa(x>>prec, buf, base, fill1);
  buf[0] = '.';
  buf++;
  x -= ((x>>prec)<<prec);
  for (int i=0;i<fill2;i++)
    x *= base;
  buf += avb_itoa(x>>prec,buf,base,fill2);
  return (buf - buf0);
}


char *avb_atoi(char *buf, int *x0)
{
  char *p = buf;
  int x = 0;
  int sign = 1;
  if (*p=='-') {
    p++;
    sign = -1;
  }
  while (*p >= '0' && *p <= '9')
    p++;
  p--;
  for(;p>=buf;p--) {
    int d = *p-'0';
    x = x * 10 + d;
  }
  x = x * sign;
  while (*p >= '0' && *p <= '9')
    p++;
  *x0 = x;
  return p;
}
