
int main () {

  OBJECT *obj_alias;
  FileStream new f;
  Symbol new s;
  
  obj_alias = f;
  printf ("%p == %p\n", (f -> instancevars), obj_alias -> instancevars);

  *s = f -> instancevars;
  printf ("%#x\n", *(s value));
}
