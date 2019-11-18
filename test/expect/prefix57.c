
void my_func (int dummy, int x) {
  printf ("%d\n", x);
}


Integer instanceMethod myMethod (void) {
  List new myList;
  struct _int_struct {
    int x;
  } int_struct;
  int y;

  myList = "one";

  int_struct.x = 0;
  y = 0;

  ++int_struct.x;

  myList map {
    ++int_struct.x;
    my_func (y, ++int_struct.x);
    printf ("%d\n", ++int_struct.x);
  }

}

int main () {

  Integer new myInt;

  myInt myMethod;
}
