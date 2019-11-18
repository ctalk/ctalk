
void my_func (int dummy, int x) {
  printf ("%d\n", x);
}


int main () {
  List new myList;
  int x, y;

  myList = "one";

  x = 0;
  y = 0;

  x++;

  myList map {
    x++;
    my_func (y, x++);
    printf ("%d\n", x++);
  }

}
