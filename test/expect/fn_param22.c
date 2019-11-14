

void my_fn (unsigned long long int_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%llu\n", int_arg);
  }
}

int main (void) {
  unsigned long long lli = 1;
  List new main_l;
  
  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (lli);
    --lli;
  }
}
