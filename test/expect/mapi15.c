/* Test a basic return expression beginning with self. */


Boolean instanceMethod myAndTrue (void) {
  Boolean new operand;
  operand = true;
  return self && operand;
}

Boolean instanceMethod myAndFalse (void) {
  Boolean new operand;
  operand = false;
  return self && operand;
}

int main (void) {
  Boolean new myBool, result;

  myBool = true;
  
  result = myBool myAndTrue;
  printf ("%d\n", result);

  result = myBool myAndFalse;
  printf ("%d\n", result);

  myBool = false;
  
  result = myBool myAndTrue;
  printf ("%d\n", result);

  result = myBool myAndFalse;
  printf ("%d\n", result);


}
