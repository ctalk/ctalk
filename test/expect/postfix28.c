
void my_func (int dummy, int x) {
  printf ("%d\n", x);
}


int main () {
  List new myList;
  struct _int_struct {
    int x;
  };
  struct _int_struct int_struct;
  struct _int_struct *i_s_p;
  int y;

  myList = "one";

  i_s_p = &int_struct;
  i_s_p -> x = 0;
  y = 0;

  i_s_p -> x++;

  myList map {
    i_s_p -> x++;
    my_func (y, i_s_p -> x++);
    printf ("%d\n", i_s_p -> x++);
  }

}
