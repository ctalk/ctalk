/* $Id: postfix1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main (int argc, char **argv) {
  int my_int;
  Character new c;
  c = 65;
  my_int = c++ asInteger;
  printf ("%d\n", my_int);
  my_int = c++ asInteger;
  printf ("%d\n", my_int);
  my_int = c++ asInteger;
  printf ("%d\n", my_int);

  c = 65;
  printf ("%d\n", c++ asInteger);
  printf ("%d\n", c++ asInteger);
  printf ("%d\n", c++ asInteger);
  
}

