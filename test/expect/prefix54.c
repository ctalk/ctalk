
void my_func (long long int dummy, long long int x) {
  printf ("%lld\n", x);
}


int main () {
  List new myList;
  long long int x, y;

  myList = "one";

  x = 0;
  y = 0;

  ++x;

  myList map {
    ++x;
    my_func (y, ++x);
    printf ("%lld\n", ++x);
  }

}
