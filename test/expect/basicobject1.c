
int main () {

  Symbol new s;
  OBJECT *obj;

  s basicObject "new_int", "Integer", "Magnitude", "10";

  printf ("%d\n", s getValue value);

  obj = s getValue;

  printf ("%d\n", obj value);

  obj = eval s getValue;

  printf ("%d\n", *(int *)obj -> instancevars -> __o_value);
}
