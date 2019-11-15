/*
 *  Also checks the tag i's in eval_expr ().
 */

int main () {

  String new s;
  String new t;

  s = "Hello, world!";

  t = s;

  printf ("%s\n", s);

  while (NULL != t++)
    printf ("%s\n", t);

  printf ("-----------------\n");

  t = s;

  printf ("%s\n", s);

  while (NULL != (t++))
    printf ("%s\n", t);

  printf ("-----------------\n");

  t = s;

  printf ("%s\n", s);

  while (NULL != (t)++)
    printf ("%s\n", t);

  printf ("-----------------\n");


}
