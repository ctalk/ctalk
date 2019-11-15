/*
 *  Like istring1.c, except this tests an extra i check in
 *  __ctalkToCInteger () (in the while () condition).
 */

int main () {

  String new s;
  String new t;

  s = "Hello, world!";

  t = s;

  printf ("%s\n", s);

  while (t++)
    printf ("%s\n", t);

  printf ("----------------\n");

  t = s;

  printf ("%s\n", s);

  while ((t++))
    printf ("%s\n", t);

  printf ("----------------\n");

  t = s;

  printf ("%s\n", s);

  while ((t)++)
    printf ("%s\n", t);


}
