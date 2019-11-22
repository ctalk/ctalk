Integer instanceMethod add2 (void) {
  methodReturnInteger(self + 2);
}

int main () {
  Integer new myInt;
  Integer new myTotal;

  myInt = 2;
  myTotal = myInt add2;
  printf ("%d\n", myTotal);
}
