
Integer instanceMethod myMethod (void) {

  String new s;
  OBJECT *s_alias;
  

  s_alias = s;
  s = "StringValue";

  printf ("%s == %s\n", s -> __o_value, s_alias -> instancevars -> __o_value);

}

int main () {
  Integer new myInt;

  myInt myMethod;
}
