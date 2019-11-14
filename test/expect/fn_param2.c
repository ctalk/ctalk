/*
 *  Test Character assignments and casts in expressions that write
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

char add_int_fn_as_char (int a, int b, int c) {
  return (char) a + b + c;
}

struct _global_int_struct {
  int global_struct_mbr;
};

int main () {

  Character new myChar;
  char my_c_char = 'c';
  int int_array[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  struct _int_struct { int int_mbr; } int_struct;
  struct _int_struct *int_struct_ptr;
  struct _global_int_struct my_global_int_struct;
  int i, total;

  if ((myChar = add_int_fn_as_char (int_fn1 (), (int)my_c_char, 1)) != 110)
    exit(1);
    
  printf ("%c ", myChar);

  if ((myChar = add_int_fn_as_char (int_fn1 (), my_c_char, 1)) != 110)
    exit(1);
    
  printf ("%c ", myChar);

  my_c_char = 'd';

  if ((myChar = add_int_fn_as_char (my_c_char, int_fn2(), 1)) != 112)
    exit(1);
    
  printf ("%c ", myChar);

  if ((myChar = add_int_fn_as_char ((int)my_c_char, int_fn2(), 1)) != 112)
    exit(1);
    
  printf ("%c ", myChar);

  my_c_char = '!';

  if ((myChar = add_int_fn_as_char (int_fn2 (), int_fn3 (), my_c_char)) != 56)
    exit(1);
    
  printf ("%c ", myChar);

  if ((myChar = add_int_fn_as_char (int_fn2 (), int_fn3 (), (int)my_c_char)) != 56)
    exit(1);
    
  printf ("%c ", myChar);

  if ((myChar = add_int_fn_as_char (int_array[1], 
				    int_fn2 (), 
				    32)) != 45)
    exit(1);
    
  printf ("%c ", myChar);

  for (i = 0; i < 10; i++) {
    total = add_int_fn_as_char (int_array[i], int_fn2 (), 32);
    if ((myChar = add_int_fn_as_char (int_array[i], 
				      int_fn2 (), 
				      32)) != total)
      exit(1);

    printf ("%c ", myChar);
  }
    
  int_struct.int_mbr = 32;

  if ((myChar = add_int_fn_as_char (int_array[1], 
				    int_fn2 (), 
				    int_struct.int_mbr)) != 45)
    exit(1);
    
  printf ("%c ", myChar);

  int_struct_ptr = &int_struct;

  if ((myChar = add_int_fn_as_char (int_array[1], 
				    int_fn2 (), 
				    int_struct_ptr -> int_mbr)) != 45)
    exit(1);
    
  printf ("%c ", myChar);

  my_global_int_struct.global_struct_mbr = 32;

  if ((myChar = add_int_fn_as_char (int_array[1], 
				    int_fn2 (), 
			    my_global_int_struct.global_struct_mbr)) != 45)
    exit(1);
    
  printf ("%c ", myChar);

  printf ("\n"); 

}
