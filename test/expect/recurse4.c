Integer instanceMethod factorial (Integer intArg) {
  Integer new localInt;
  intArg = intArg + 1;
  localInt become intArg;
  printf ("%d\n", localInt);
  if ((self == 0) || (self == 1)) {
    methodReturnInteger(1);
  }
  methodReturnObject(self * (self - 1) factorial (localInt));
}

int main () {
  Integer new i;
  Integer new arg;
  arg = 1;
  printf ("%d\n", arg);
  i = 1;
  printf ("1! = %d\n", i factorial (arg));
  i = 2;
  printf ("2! = %d\n", i factorial (arg));
  i = 3;
  printf ("3! = %d\n", i factorial (arg));
  i = 4;
  printf ("4! = %d\n", i factorial (arg));
  i = 5;
  printf ("5! = %d\n", i factorial (arg));
  i = 6;
  printf ("6! = %d\n", i factorial (arg));

  printf ("1! = %d\n", 1 factorial (arg));
  printf ("2! = %d\n", 2 factorial (arg));
  printf ("3! = %d\n", 3 factorial (arg));
  printf ("4! = %d\n", 4 factorial (arg));
  printf ("5! = %d\n", 5 factorial (arg));
  printf ("6! = %d\n", 6 factorial (arg));
  printf ("%d\n", arg);
}
