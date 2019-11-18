
int main () {

  String new s;
  OBJECT *s_alias;
  

  s = "StringValue";
  s_alias = s;

  printf ("%s == %s\n", s -> __o_value, s_alias -> instancevars -> __o_value);

}
