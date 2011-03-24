#include <stdio.h>


int main()
{
  char c;
  int i;
  FILE *fp =  fopen("out.txt", "w");

  if (fp == NULL)
  {
    fprintf(stderr, "Can't open output file !\n");
    exit(1);
  }

  c = ' ';

  while(c <= '~')
  {
    fprintf(fp, "%c", c);
    c++;
  }

  return 0;
}
