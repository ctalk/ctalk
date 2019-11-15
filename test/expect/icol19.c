
List new myList;

#define BASE_COEFF 2000

Application instanceMethod myMethod (Integer param_int) {
  Key new k;

  /* This is the expression we want to check. */
  k = myList + (param_int - BASE_COEFF);

  printf ("%s\n", *k);
}

int main () {
  Symbol new listPtr;
  Application new myApp;

  *listPtr = String basicNew "String1", "String1";
  myList push *listPtr;
  *listPtr = String basicNew "String2", "String2";
  myList push *listPtr;
  *listPtr = String basicNew "String3", "String3";
  myList push *listPtr;

  myApp myMethod (BASE_COEFF + 2);
}
