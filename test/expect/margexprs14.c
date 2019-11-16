/* $Id: margexprs14.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  As many uncontrived examples of prefix ++ operators as I
 *  can think of.
 */

int main (int argc, char **argv) {
  int idx_int;
  String new stringObject;
  Character new charObject;
  stringObject = "abcd";

  idx_int = 0;
  charObject = stringObject at idx_int;
  printf ("%c\n", charObject);
  charObject = stringObject at ++idx_int;
  printf ("%c\n", charObject);
  charObject = stringObject at ++idx_int;
  printf ("%c\n", charObject);
  charObject = stringObject at ++idx_int;
  printf ("%c\n", charObject);

  idx_int = 0;
  printf ("%c\n", stringObject at idx_int);
  printf ("%c\n", stringObject at ++idx_int);
  printf ("%c\n", stringObject at ++idx_int);
  printf ("%c\n", stringObject at ++idx_int);
}
