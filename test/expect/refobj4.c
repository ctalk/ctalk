
int main () {

  String new s;
  String new s_start;
  Integer new i_start;
  OBJECT *i_alias;
  OBJECT *s_alias;
  

  s_alias = s;
  s = "StringValue";
  i_alias = i_start;
  i_start = 6;

  s_start = s_alias subString i_alias, 5;

  printf ("%s\n", s_start);
}
