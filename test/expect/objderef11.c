
int main () {
  String new myObject;
  Symbol new optr;
  String new s;
  
  *optr = myObject -> __o_classname;

  printf ("%s\n", *optr);
  printf ("%s\n", myObject -> __o_classname);

  /* this needs work */
  /* *optr = myObject -> CLASSNAME; */

  /* printf ("%s\n", *optr);
     printf ("%s\n", myObject -> CLASSNAME); */ 
  *optr = myObject;
  printf ("%s\n", (*optr) className);
  printf ("%s\n", myObject className);

  s = myObject -> __o_classname;
  printf ("%s\n", s);
  /* so does this */
  /* s = myObject -> CLASSNAME; */
  s = myObject className;
  printf ("%s\n", s);
}
