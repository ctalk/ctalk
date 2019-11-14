

void my_fn (long double d_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%Lf\n", d_arg);
  }
}

int main (void) {
  long double d_arg = 1.0;
  List new main_l;
  
  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (d_arg);
    --d_arg;
  }
}
