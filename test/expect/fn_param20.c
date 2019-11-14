

void my_fn (unsigned long int_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%lu\n", int_arg);
  }
}

int main (void) {
  unsigned long lu = 1;
  List new main_l;
  
  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (lu);
    --lu;
  }
}
