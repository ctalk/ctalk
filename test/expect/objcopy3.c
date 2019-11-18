
/*
 *  Test global object aliasing.
 */
String new s1;
String new s2;

int main (int argc, char **argv) {

  s1 = "Array s1 value.";
  s2 copy s1;
  printf ("Copy of %s\n", s2);

}
