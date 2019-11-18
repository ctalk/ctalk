
int main () {

  OBJECT *class_alias;
  FileStream new f;
  
  class_alias = __ctalkGetClass ("FileStream");
  printf ("%p == %p\n", f -> classvars, class_alias -> classvars);

}
