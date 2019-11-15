/* $Id: if1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test an if expression with a C data type and an object.
 */

int main (char **argv, int argc) {

  Integer new i;
  Integer new k;
  int j;

  i = 10;
  j = 9;
  k = 8;

  if (j < i) 
    printf ("j < i\n");
  else
    printf ("i < j\n");
  
  if (i < j)
    printf ("i < j\n");
  else
    printf ("j < i\n");

  if (k < i)
    printf ("k < i\n");
  else
    printf ("i < k\n");

  if (i < k)
    printf ("i < k\n");
  else
    printf ("k < i\n");

  exit(0);
}
