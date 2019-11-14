

void my_fn (int int_arg) {
  List new l;

  l = "one", "two", "three";

  l map {
    printf ("%d\n", int_arg);
  }
}

int main (void) {

  my_fn (3);
}
