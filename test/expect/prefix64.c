
void my_func (int dummy, int x) {
  printf ("%d\n", x);
}


Integer instanceMethod myMethod (void) {
  List new myList;
  int x[5];
  int y;

  myList = "one";

  x[1] = 1;
  y = 0;

  ++x[1];

  myList map {
    my_func (y, ++x[1]);
  }

}

int main () {

  Integer new myInt;

  myInt myMethod;
}
