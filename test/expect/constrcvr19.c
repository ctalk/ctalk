/*
 *  Receiver expressions.
 */

int main () {
  String new s;
  Integer new i;
  char s1[255];
  char s2[255];
  char s3[255];
  xstrcpy (s1, "Hello, ");
  xstrcpy (s2, "world ");
  xstrcpy (s3, "again!");
  printf ("%d\n", (("Hello, " + "world ") + "again!") length);
  s = (("Hello, " + "world ") + "again!");
  i = s length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + "world ") + "again!") length);
  s = ((s1 + "world ") + "again!");
  i = s length;
  printf ("%d\n", i);
  printf ("%d\n", (("Hello, " + s2) + "again!") length);
  s = (("Hello, " + s2) + "again!");
  i = s length;
  printf ("%d\n", i);
  printf ("%d\n", (("Hello, " + "world ") + s3) length);
  s = (("Hello, " + "world ") + s3);
  i = s length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + s2) + "again!") length);
  s = ((s1 + s2) + "again!");
  i = s length;
  printf ("%d\n", i);
  printf ("%d\n", (("Hello, " + s2) + s3) length);
  s = (("Hello, " + s2) + s3);
  i = s length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + "world ") + s3) length);
  s = ((s1 + "world ") + s3);
  i = s length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + s2) + s3) length);
  s = ((s1 + s2) + s3);
  i = s length;
  printf ("%d\n", i);
}
