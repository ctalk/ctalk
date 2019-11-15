/* $Id: if2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test if expressions with C functions.
 */

int int_fn1 (void) {
  return 10;
}

int int_fn2 (void) {
  return 11;
}

int int_fn3 (void) {
  return 12;
}

int add_int_fn (int a, int b, int c) {
  return a + b + c;
}

char add_int_fn_as_char (int a, int b, int c) {
  return (char) a + b + c;
}

char *add_int_fn_as_string (int a, int b, int c) {
  static char buf[30];
  sprintf (buf, "%d", a + b + c);
  return buf;
}

double add_int_fn_as_double (int a, int b, int c) {
  return (double)a + b + c;
}

int main () {

  Integer new myInt1;
  Integer new myInt2;
  Integer new myInt3;
  Integer new myInt4;
  Integer new myInt5;
  Integer new myInt6;
  Integer new myInt7;
  Character new myChar1;
  String new myString1;
  Float new myFloat1;

  if ((myChar1 = add_int_fn_as_char (int_fn1 (), 20, 30)) != 60)
    exit(1);
    
  printf ("%c ", myChar1);

  if ((myString1 = add_int_fn_as_string (int_fn1 (), 20, 30)) != "60")
    exit(1);
    
  printf ("%s ", myString1);

  if ((myFloat1 = add_int_fn_as_double (int_fn1 (), 2, 3)) != 15.0)
    exit(1);
    
  printf ("%lf ", myFloat1);

  if ((myInt1 = add_int_fn (int_fn1 (), 2, 3)) != 15)
    exit(1);
    
  if ((myInt2 = add_int_fn (1, int_fn2 (), 3)) != 15)
    exit(1);

  if ((myInt3 = add_int_fn (1, 2, int_fn3 ())) != 15)
    exit(1);

  if ((myInt4 = add_int_fn (int_fn1 (), int_fn2 (), 3)) != 24)
    exit(1);

  if ((myInt5 = add_int_fn (int_fn1 (), 2, int_fn3 ())) != 24)
    exit(1);

  if ((myInt6 = add_int_fn (1, int_fn2 (), int_fn3 ())) != 24)
    exit(1);

  if ((myInt7 = add_int_fn (int_fn1 (), int_fn2 (), int_fn3 ())) != 33)
    exit(1);

  printf ("%d %d %d %d %d %d %d\n", 
	  myInt1, myInt2, myInt3, myInt4, myInt5, myInt6, myInt7);

}
