/* an output like, "3 == 1 == 3" is correct. (The second printf
 arg is a C expression). */

int main () {

  String new s;
  OBJECT *s_alias;
  Integer new nrefsInt;
  

  s_alias = s;

  nrefsInt = s -> nrefs;

  printf ("%d == %d == %d\n", s -> nrefs, s_alias -> nrefs, nrefsInt);

}
