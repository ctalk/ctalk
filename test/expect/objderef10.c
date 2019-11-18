
int main () {
  Object new myObject;
  Symbol new optr;
  String new s;
  
  *optr = myObject -> __o_name;

  printf ("%s\n", *optr);
  printf ("%s\n", myObject -> __o_name);

  s = myObject -> __o_name;
  printf ("%s\n", s);
}
