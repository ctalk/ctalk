
void my_func (long int dummy, long int x) {
  printf ("%ld\n", x);
}


int main () {
  List new myList;
  long int x, y;

  myList = "one";

  x = 0;
  y = 0;

  x++;

  myList map {
    x++;
    my_func (y, x++);
    printf ("%ld\n", x++);
  }

}
