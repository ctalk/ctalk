
Character instanceMethod printChr (void) {
  printf ("%d\n", self asInteger);
}

String instanceMethod myChrs (void) {
  int i;
  Integer new chrVal;

  for (i = 0; i < self length; ++i) {
    chrVal = (self at i) asInteger;
    printf ("%d\n", chrVal);
  }

  for (i = 0; i < self length; ++i)
    printf ("%d\n", (self at i) asInteger);
  
  for (i = 0; i < self length; ++i)
    (self at i) printChr;

}


int main (void) {
  String new s;
  
  s = "Hello, world!";
  s myChrs;
}
