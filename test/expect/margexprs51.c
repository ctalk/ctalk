
int my_int_func (void) {
  return 2;
}

long int my_long_int_func (void) {
  return 3;
}

long long int my_long_long_int_func (void) {
  return 4;
}

int main () {
  Integer new myInt, myResult;

  myInt = 1;

  myResult = myInt + my_int_func ();
  
  printf ("%d\n", myResult);

  myResult = myInt + my_int_func () + my_long_int_func ();
  
  printf ("%d\n", myResult);

  myResult = myInt + my_int_func () + my_long_int_func () +
    my_long_long_int_func ();
  
  printf ("%d\n", myResult);

}
