

void my_fn (unsigned int int_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%u\n", int_arg);
  }
}

int main (void) {
  unsigned int ui = 1;
  List new main_l;

  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (ui);
    --ui;
  }
}
