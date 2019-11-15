/*
 *  This basically checks make_tag_for_varentry_active (), 
 *  __ctalkIncStringRef (), and the i handling in __ctalkToCCharPtr ().
 */

int main () {

  String new s;
  String new t;

  s = "Hello, world!";

  t = s;

  ++t;

  printf ("%s\n", s);
  printf ("%s\n", t);

  ++t;

  printf ("%s\n", t);

  ++t;
  printf ("%s\n", t);

  ++t;
  printf ("%s\n", t);

  ++t;
  printf ("%s\n", t);
  ++t;
  printf ("%s\n", t);
  ++t;
  printf ("%s\n", t);
  ++t;
  printf ("%s\n", t);

  ++t;
  printf ("%s\n", t);
  ++t;
  printf ("%s\n", t);
  ++t;
  printf ("%s\n", t);
  ++t;
  printf ("%s\n", t);

  ++t;
  printf ("%s\n", t);

  /* What happens past the end of the value should be undefined. */
  /*  ++t;
      printf ("%s\n", t); */

  printf ("------------------\n");

  t = s;

  (++t);

  printf ("%s\n", s);
  printf ("%s\n", t);

  (++t);

  printf ("%s\n", t);

  (++t);
  printf ("%s\n", t);

  (++t);
  printf ("%s\n", t);

  (++t);
  printf ("%s\n", t);
  (++t);
  printf ("%s\n", t);
  (++t);
  printf ("%s\n", t);
  (++t);
  printf ("%s\n", t);

  (++t);
  printf ("%s\n", t);
  (++t);
  printf ("%s\n", t);
  (++t);
  printf ("%s\n", t);
  (++t);
  printf ("%s\n", t);

  (++t);
  printf ("%s\n", t);

  /* What happens past the end of the value should be undefined. */
  /*  (++t);
      printf ("%s\n", t); */

  printf ("------------------\n");

  t = s;

  ++(t);

  printf ("%s\n", s);
  printf ("%s\n", t);

  ++(t);

  printf ("%s\n", t);

  ++(t);
  printf ("%s\n", t);

  ++(t);
  printf ("%s\n", t);

  ++(t);
  printf ("%s\n", t);
  ++(t);
  printf ("%s\n", t);
  ++(t);
  printf ("%s\n", t);
  ++(t);
  printf ("%s\n", t);

  ++(t);
  printf ("%s\n", t);
  ++(t);
  printf ("%s\n", t);
  ++(t);
  printf ("%s\n", t);
  ++(t);
  printf ("%s\n", t);

  ++(t);
  printf ("%s\n", t);

  /* What happens past the end of the value should be undefined. */
  /* ++(t);
     printf ("%s\n", t); */

  printf ("------------------\n");
}
