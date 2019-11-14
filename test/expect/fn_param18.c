

void my_fn (unsigned short int_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%hu\n", int_arg);
  }
}

int main (void) {
  unsigned short ui = 1;
  List new main_l;
  
  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (ui);
    --ui;
  }
}
