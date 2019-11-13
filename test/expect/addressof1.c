
int main () {

  Object new o;
  Symbol new s;
  OBJECT *o_alias, *s_alias;

  *s = o addressOf;

  printf ("These two addresses should be identical.\n");

  /* 
   *  This is probably *not* the correct semantics for 
   *  eval (), even if we put it in the context of printing
   *  a pointer.
   */

  /* printf ("%p\n", eval o); */


  o_alias = eval o;
  s_alias = s;

  /* This is better. */
  printf ("%p\n", &o);
  printf ("%p\n", s value);

}
