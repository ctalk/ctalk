

void my_fn (float d_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%f\n", d_arg);
  }
}

int main (void) {
  float d_arg = 1.0;
  List new main_l;
  
  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (d_arg);
    --d_arg;
  }
}
