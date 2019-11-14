/*
 *  Receiver expressions with char *'s.
 */

int main () {
  int i;
  char s1[MAXLABEL];
  char s2[MAXLABEL];
  char s3[MAXLABEL];
  char s4[MAXLABEL];

  xstrcpy (s1, "Hello, ");
  xstrcpy (s2, "world ");
  xstrcpy (s3, "again");
  xstrcpy (s4, "!");

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
