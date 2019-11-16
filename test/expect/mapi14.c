/* Test a basic return expression beginning with self. */


Boolean instanceMethod myOr (Boolean b) {
  return self || b;
}

int main (void) {
  Boolean new myBool, result;

  myBool = true;
  
  result = myBool myOr true;
  printf ("%d\n", result);

  result = myBool myOr false;
  printf ("%d\n", result);

  myBool = false;
  
  result = myBool myOr true;
  printf ("%d\n", result);

  result = myBool myOr false;
  printf ("%d\n", result);


}
