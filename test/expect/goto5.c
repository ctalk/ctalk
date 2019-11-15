
Integer instanceMethod myMethod (void) {

  List new l;


  l push 1;
  l push 2;
  l push 3;
  l push 4;
  l push 5;

 loop_again:
  l map {
    if (self > 3) {
      goto loop_again;
    }
    printf ("%d\n", self);
  }

  return (0);
}

int main () {
  Integer new myInt;

  myInt myMethod;
}
