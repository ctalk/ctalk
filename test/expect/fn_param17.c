

void my_fn (short int_arg) {
  List new l;

  l = "one", "two", "three";

  l map {
    printf ("%hi\n", int_arg);
  }
}

int main (void) {

  my_fn ((short)3);
}
