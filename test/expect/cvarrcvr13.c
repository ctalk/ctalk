/* $Id: cvarrcvr13.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like cvarrcvr2.c, with struct member derefs and local, separate
 *  struct definition and struct tag declarations.
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
  struct __s {
    char *s;
  };
  struct __s s_struct, *s_ptr;

  s_ptr = &s_struct;
  xstrcpy (str, "Hello, world!");
  s_struct.s = str;
  my_int = s_ptr -> s initialChar;
  printf ("%d\n", my_int);
  ++s_ptr -> s;
  my_int = s_ptr -> s initialChar;
  printf ("%d\n", my_int);
  ++s_ptr -> s;
  my_int = s_ptr -> s initialChar;
  printf ("%d\n", my_int);
}

