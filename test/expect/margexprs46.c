Application instanceMethod myAdd (Integer myInt) {
  int result;
  int operand;

  operand = 3;

  result = myInt + operand;
  printf ("%d\n", result);
}

int main () {

  Application new myApplication;

  myApplication myAdd 3;

}
