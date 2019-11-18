
int main () {

  String new myObject;
  Symbol new optr;
  String new s;
  
  *optr = myObject -> __o_superclassname;

  printf ("%s\n", *optr);
  printf ("%s\n", myObject -> __o_superclassname);

  /* this needs work */
  /* *optr = myObject -> SUPERCLASSNAME; */

  /* printf ("%s\n", *optr);
  printf ("%s\n", myObject -> SUPERCLASSNAME); */

  *optr = myObject;
  printf ("%s\n", (*optr) superclassName);
  printf ("%s\n", myObject superclassName);

    s = myObject -> __o_superclassname;
  printf ("%s\n", s);
  /* this needs work too */
  /* s = myObject -> SUPERCLASSNAME; */
  s = myObject superclassName;
  printf ("%s\n", s);
}
