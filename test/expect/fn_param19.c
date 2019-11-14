

void my_fn (long int_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%ld\n", int_arg);
  }
}

int main (void) {
  long li = 1;
  List new main_l;
  
  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (li);
    --li;
  }
}
