
int main () {

  String new s;
  String new t;
  Integer new halfLength;
  Integer new i;

  s = "Hello, world!";

  i = 0;
  halfLength = 7;

  t = s;

  printf ("%s\n", s);

  while (++i <= halfLength)
    printf ("%s\n", t++);

  while (--t != NULL)
    printf ("%s\n", t);

  printf ("----------------\n");

  i = 0;
  t = s;

  printf ("%s\n", s);

  while (++i <= halfLength)
    printf ("%s\n", (t++));

  while ((--t) != NULL)
    printf ("%s\n", t);

  printf ("----------------\n");

  i = 0;
  t = s;

  printf ("%s\n", s);

  while (++i <= halfLength)
    printf ("%s\n", (t)++);

  while (--(t) != NULL)
    printf ("%s\n", t);

  printf ("----------------\n");

}
