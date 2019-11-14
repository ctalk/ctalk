/*
 *  Receiver expressions with String constants.
 */

int main () {
  Integer new i;

  i = (("Hello, " + "world ") + "again" + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", (("Hello, " + "world ") + "again" + "!") length);

  i = ((("Hello, " + "world ") + "again") + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", ((("Hello, " + "world ") + "again") + "!") length);

  /* Generates an invalid operand exception. */
/*   i = ("Hello, " + "world ") + ("again" + "!") length; */
/*   printf ("%d\n", i); */
/*   printf ("%d\n", ("Hello, " + "world ") + ("again" + "!") length); */

  i = (("Hello, " + "world ") + ("again" + "!")) length;
  printf ("%d\n", i);
  printf ("%d\n", (("Hello, " + "world ") + ("again" + "!")) length);

  i = ("Hello, " + ("world " + "again" + "!")) length;
  printf ("%d\n", i);
  printf ("%d\n", ("Hello, " + ("world " + "again" + "!")) length);

  i = ("Hello, " + "world " + ("again" + "!")) length;
  printf ("%d\n", i);
  printf ("%d\n", ("Hello, " + "world " + ("again" + "!")) length);

  i = ("Hello, " + "world " + "again" + "!") length;
  printf ("%d\n", i);
  printf ("%d\n", ("Hello, " + "world " + "again" + "!") length);

}
