

void my_fn (double d_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%lf\n", d_arg);
  }
}

int main (void) {
  double d_arg = 1.0;
  List new main_l;
  
  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (d_arg);
    --d_arg;
  }
}
