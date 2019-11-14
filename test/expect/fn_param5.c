/*
 *  Test Float assignments and casts in expressions that write
 *  user templates.
 */

double double_fn1 (void) {
  return 10.0;
}

double double_fn2 (void) {
  return 11.0;
}

double double_fn3 (void) {
  return 12.0;
}

double add_double_fn (double a, double b, double c) {
  return a + b + c;
}

int main () {

  Float new myFloat;
  char my_c_char = 'c';

  if ((myFloat = add_double_fn (double_fn1 (), 
				(double)my_c_char, 
				1.0)) != 110.0)
    exit(1);
    
  printf ("%lf ", myFloat);

  if ((myFloat = add_double_fn (double_fn1 (),
				  my_c_char,
				  1.0)) != 110.0)
    exit(1);
    
  printf ("%lf ", myFloat);

  my_c_char = 'd';

  if ((myFloat = add_double_fn (my_c_char, double_fn2(), 1.0)) != 112.0)
    exit(1);
    
  printf ("%lf ", myFloat);

  if ((myFloat = add_double_fn ((int)my_c_char, double_fn2(), 
				1.0)) != 112.0)
    exit(1);
    
  printf ("%lf ", myFloat);

  my_c_char = '!';

  if ((myFloat = add_double_fn (double_fn2 (), double_fn3 (),
				my_c_char)) != 56.0)
    exit(1);
    
  printf ("%lf ", myFloat);

  if ((myFloat = add_double_fn (double_fn2 (),
				double_fn3 (),
				(int)my_c_char)) != 56.0)
    exit(1);
    
  printf ("%lf ", myFloat);

  if ((myFloat = add_double_fn (double_fn1 (),
				double_fn2 (),
				double_fn3 ())) != 33.0)
    exit(1);
    
  printf ("%lf ", myFloat);

  printf ("\n"); 

}
