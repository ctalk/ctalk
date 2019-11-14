/*
 *  Receiver expressions with Strings.
 */

int main () {
  String new s1;
  String new s2;
  String new s3;
  String new s4;
  Integer new i;

  s1 = "Hello, ";
  s2 = "world ";
  s3 = "again";
  s4 = "!";

  i = ((s1 + "world ") + "again" + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + "world ") + "again" + "!") length);

  i = (("Hello, " + s2) + "again" + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", (("Hello, " + s2) + "again!") length);

  i = (("Hello, " + "world ") + s3 + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", (("Hello, " + "world ") + s3 + "!") length);

  i = (("Hello, " + "world ") + "again" + s4) length;
  printf ("%d\n", i);
  printf ("%d\n", (("Hello, " + "world ") + "again" + s4) length);

  i = ((s1 + s2) + "again" + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + s2) + "again" + "!") length);

  i = ((s1 + "world ") + s3 + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + "world ") + s3 + "!") length);

  i = ((s1 + "world ") + "again" + s4) length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + "world ") + "again" + s4) length);

  i = ((s1 + s2) + s3 + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + s2) + s3 + "!") length);

  i = ((s1 + s2) + "again" + s4) length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + s2) + "again" + s4) length);

  i = ((s1 + s2) + s3 + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + s2) + s3 + "!") length);

  i = ((s1 + s2) + s3 + s4) length;
  printf ("%d\n", i);
  printf ("%d\n", ((s1 + s2) + s3 + s4) length);
}
