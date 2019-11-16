
int my_int_func (int arg) {
  return arg;
}

long int my_long_int_func (void) {
  return 3;
}

long long int my_long_long_int_func (int arg) {
  return arg;
}

int main () {
  int int_arg = 4;
  Integer new myInt, myResult, myArg;

  myInt = 1;
  myArg = 2;

  myResult = myInt + my_int_func (myArg);
  
  printf ("%d\n", myResult);

  myResult = myInt + my_int_func (myArg) + my_long_int_func ();
  
  printf ("%d\n", myResult);

  myResult = myInt + my_int_func (myArg) + my_long_int_func () +
    my_long_long_int_func (int_arg);
  
  printf ("%d\n", myResult);

}
