/* $Id: cvarrcvr24.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like cvarrcvr18.c, with anonymous struct members plus tag
 *  plus postfix op.
 */

String instanceMethod initialChar (void) {
  char *s;
  returnObjectClass Character;
  s = self value;
  return *s;
}

int main (int argc, char **argv) {
  int my_int;
  char str[25];
  struct {
    char *s;
  } s_struct;
  xstrcpy (str, "Hello, world!");
  s_struct.s = str;
  my_int = s_struct.s++ initialChar;
  printf ("%d\n", my_int);
  my_int = s_struct.s++ initialChar;
  printf ("%d\n", my_int);
  my_int = s_struct.s initialChar;
  printf ("%d\n", my_int);
}

