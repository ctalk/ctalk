/* $Id: cvarrcvr23.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like cvarrcvr8.c, with anonymous struct definition and struct tag 
 *  declarations, plus prefix op plus parenthesized lvalue.
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
  } s_struct, *s_ptr;

  s_ptr = &s_struct;
  xstrcpy (str, "Hello, world!");
  s_ptr -> s = str;
  my_int = ++s_ptr -> s initialChar;
  printf ("%d\n", my_int);
  my_int = ++s_ptr -> s initialChar;
  printf ("%d\n", my_int);
  my_int = (s_ptr -> s) initialChar;
  printf ("%d\n", my_int);

  s_ptr -> s = str;
  printf ("%c\n", ++s_ptr -> s initialChar);
  printf ("%c\n", ++s_ptr -> s initialChar);
  printf ("%c\n", s_ptr -> s initialChar);

  s_ptr -> s = str;
  my_int = ++(s_ptr -> s) initialChar;
  printf ("%d\n", my_int);
  my_int = ++(s_ptr -> s) initialChar;
  printf ("%d\n", my_int);
  my_int = (s_ptr -> s) initialChar;
  printf ("%d\n", my_int);

  s_ptr -> s = str;
  printf ("%c\n", ++(s_ptr -> s) initialChar);
  printf ("%c\n", ++(s_ptr -> s) initialChar);
  printf ("%c\n", (s_ptr -> s) initialChar);
}

