

/*
 *  Object copy using function's local objects.
 */

int main (int argc, char **argv) {

  String new s1;
  String new s2;

  s1 = "Array s1 value.";
  s2 copy s1;
  printf ("Copy of %s\n", s2);

}
