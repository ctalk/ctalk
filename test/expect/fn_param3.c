/*
 *  Test Integer assignments and casts in expressions that write
 *  user templates.
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

int main () {

  Integer new myInt;
  char my_c_char = 'c';

  if ((myInt = add_int_fn (int_fn1 (), (int)my_c_char, 1)) != 110) {
    printf ("%d\n", myInt);
    exit(1);
  }
    
  printf ("%d ", myInt);

  if ((myInt = add_int_fn (int_fn1 (), my_c_char, 1)) != 110)
    exit(1);
    
  printf ("%d ", myInt);

  my_c_char = 'd';

  if ((myInt = add_int_fn (my_c_char, int_fn2(), 1)) != 112)
    exit(1);
    
  printf ("%d ", myInt);

  if ((myInt = add_int_fn ((int)my_c_char, int_fn2(), 1)) != 112)
    exit(1);
    
  printf ("%d ", myInt);

  my_c_char = '!';

  if ((myInt = add_int_fn (int_fn2 (), int_fn3 (), my_c_char)) != 56)
    exit(1);
    
  printf ("%d ", myInt);

  if ((myInt = add_int_fn (int_fn2 (), int_fn3 (), (int)my_c_char)) != 56)
    exit(1);
    
  printf ("%d ", myInt);

  if ((myInt = add_int_fn (int_fn1 (), int_fn2 (), int_fn3 ())) != 33)
    exit(1);
    
  printf ("%d ", myInt);

  printf ("\n"); 

}
