/*
 *  Test LongInteger assignments and casts in expressions that write
 *  user templates.
 */

long long int longlong_int_fn1 (void) {
  return 10;
}

long long int longlong_int_fn2 (void) {
  return 11;
}

long long int longlong_int_fn3 (void) {
  return 12;
}

long long int add_long_long_int_fn (long long int a, 
				    long long int b, 
				    long long int c) {
  return a + b + c;
}

int main () {

  LongInteger new myLongInt;
  char my_c_char = 'c';

  if ((myLongInt = add_long_long_int_fn (longlong_int_fn1 (), 
					 (long long int)my_c_char, 
					 1ll)) != 110)
    exit(1);
    
  printf ("%lldll ", myLongInt);

  if ((myLongInt = add_long_long_int_fn (longlong_int_fn1 (), 
					 my_c_char, 
					 1ll)) != 110)
    exit(1);
    
  printf ("%lldll ", myLongInt);

  my_c_char = 'd';

  if ((myLongInt = add_long_long_int_fn (my_c_char, 
					 longlong_int_fn2(), 
					 1ll)) != 112)
    exit(1);
    
  printf ("%lldll ", myLongInt);

  if ((myLongInt = add_long_long_int_fn ((int)my_c_char, 
					 longlong_int_fn2(), 
					 1ll)) != 112)
    exit(1);
    
  printf ("%lldll ", myLongInt);

  my_c_char = '!';

  if ((myLongInt = add_long_long_int_fn (longlong_int_fn2 (), 
					 longlong_int_fn3 (), 
					 my_c_char)) != 56)
    exit(1);
    
  printf ("%lldll ", myLongInt);

  if ((myLongInt = add_long_long_int_fn (longlong_int_fn2 (), 
					 longlong_int_fn3 (), 
					 (int)my_c_char)) != 56)
    exit(1);
    
  printf ("%lldll ", myLongInt);

  if ((myLongInt = add_long_long_int_fn (longlong_int_fn1 (), 
					 longlong_int_fn2 (), 
					 longlong_int_fn3 ())) != 33)
    exit(1);
    
  printf ("%lldll ", myLongInt);

  printf ("\n"); 

}
