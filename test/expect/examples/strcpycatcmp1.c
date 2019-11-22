int main () {
  String new s1;
  String new s2;
  String new helloString;

  /* C's strcpy () would also work here. */
  s1 = "Hello, ";
  s2 = "world!";

  /* And strcat () could perform the same task here. */
  helloString = s1 + s2;

  printf ("The value of \"helloString\" is \"%s\".\n", helloString);

  s1 = "s1";
  s2 = "s2";

  /* Test whether String objects are equal without strcmp (). */
  if (s1 == s2) {
    printf ("The strings s1 and s2 are equal.\n");
  } else {
    printf ("The strings s1 and s2 are not equal.\n");
  }
}
