
int main () {

  String new myObject;
  Symbol new optr;
  Object new classObj;
  
  myObject = "String";  /* Needed for classObject method. */

  printf ("%p\n", myObject -> __o_class);
  /* printf ("%s\n", myObject -> __o_class -> __o_name); */
  printf ("%s\n", myObject className);

  *optr = myObject -> __o_class;
  printf ("%p\n", (*optr) -> __o_class);
  /* needs work */
  /* printf ("%s\n", (*optr) -> __o_class -> __o_name); */
  /* printf ("%s\n", (*optr) className); */
  printf ("%s\n", myObject className);
  

  classObj = (*optr) classObject;
  printf ("%p\n", classObj);
  /* this needs work, too */
  /* printf ("%s\n", classObj -> __o_name); */
  printf ("%s\n", myObject className);

  classObj = myObject classObject;
  printf ("%p\n", classObj);
  printf ("%s\n", classObj -> __o_name);

}
