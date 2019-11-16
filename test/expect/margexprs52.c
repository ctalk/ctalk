
float my_float_func (void) {
  return 2.0;
}

double my_double_func (void) {
  return 3.0;
}

int main () {
  Float new myFloat, myResult;

  
  myFloat = 1.0;

  myResult = myFloat + my_float_func ();
  
  printf ("%lf\n", myResult);

  myResult = myFloat + my_double_func () + my_float_func ();
  
  printf ("%lf\n", myResult);

}
