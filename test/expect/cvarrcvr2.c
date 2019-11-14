/* $Id: cvarrcvr2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Here to make sure that a simple receiver expression works.
 */

int main (int argc, char **argv) {
  int my_int;
  char str[25], *s;
  xstrcpy (str, "1234");
  s = str;
  my_int = s asInteger;
  printf ("%d\n", my_int);
  ++s;
  my_int = s asInteger;
  printf ("%d\n", my_int);
  ++s;
  my_int = s asInteger;
  printf ("%d\n", my_int);
}

