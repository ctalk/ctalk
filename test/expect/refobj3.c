
int main () {

  String new s;
  String new s_start;
  Integer new i_start;
  OBJECT *i_alias;
  

  s = "StringValue";
  i_alias = i_start;
  i_start = 6;

  s_start = s subString i_alias, 5;

  printf ("%s\n", s_start);
}
