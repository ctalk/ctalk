

void my_fn (long long int_arg) {
  List new l;

  l = "one";

  l map {
    printf ("%lld\n", int_arg);
  }
}

int main (void) {
  long long lli = 1;
  List new main_l;
  
  main_l = "one", "two", "three", "four";

  main_l map {
    my_fn (lli);
    --lli;
  }
}
