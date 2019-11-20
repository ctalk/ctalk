Integer instanceMethod factorial (void) {
  Integer new selfMinusOne;
  if ((self == 0) || (self == 1)) {
    methodReturnInteger(1);
  }
  selfMinusOne = self - 1;
  methodReturnObject(self * selfMinusOne factorial);
}

int main () {
  Integer new i;
  i = 1;
  printf ("1! = %d\n", i factorial);
  i = 2;
  printf ("2! = %d\n", i factorial);
  i = 3;
  printf ("3! = %d\n", i factorial);
  i = 4;
  printf ("4! = %d\n", i factorial);
  i = 5;
  printf ("5! = %d\n", i factorial);
  i = 6;
  printf ("6! = %d\n", i factorial);

  printf ("1! = %d\n", 1 factorial);
  printf ("2! = %d\n", 2 factorial);
  printf ("3! = %d\n", 3 factorial);
  printf ("4! = %d\n", 4 factorial);
  printf ("5! = %d\n", 5 factorial);
  printf ("6! = %d\n", 6 factorial);
}
