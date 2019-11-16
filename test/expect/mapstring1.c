/* This should only print the reverse-cased "Hello" (calculate the
   values of ',' ^ 32 and ' ' ^ 32 and see why). */

int main () {

  String new s;

  s = "Hello, world!";
  
  s map {
    (Character *)self ^= 32;
  }

  printf ("%s\n", s);
}
