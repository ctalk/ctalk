/*
 *  Receiver expressions.
 */

int main () {
  String new s;
  String new s1;
  String new s2;
  String new s3;
  Integer new i;
  s = (("Hello, " + "world ") + "again!");
  i = s length;
  printf ("%d\n", i);

  printf ("%d\n", (("Hello, " + "world ") + "again!") length);
  s1 = "Hello, ";
  s2 = "world ";
  s3 = "again!";
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
