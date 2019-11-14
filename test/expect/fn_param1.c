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

  Integer new myInt1;
  Integer new myInt2;
  Integer new myInt3;
  Integer new myInt4;
  Integer new myInt5;
  Integer new myInt6;
  Integer new myInt7;

  myInt1 = add_int_fn (int_fn1 (), 2, 3);
  myInt2 = add_int_fn (1, int_fn2 (), 3);
  myInt3 = add_int_fn (1, 2, int_fn3 ());
  myInt4 = add_int_fn (int_fn1 (), int_fn2 (), 3);
  myInt5 = add_int_fn (int_fn1 (), 2, int_fn3 ());
  myInt6 = add_int_fn (1, int_fn2 (), int_fn3 ());
  myInt7 = add_int_fn (int_fn1 (), int_fn2 (), int_fn3 ());

  printf ("%d %d %d %d %d %d %d\n", 
	  myInt1, myInt2, myInt3, myInt4, myInt5, myInt6, myInt7);

}
