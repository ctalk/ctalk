/* This release doesn't yet use _makeRef/_getRef for "self," but
   the test is here anyway.... */

String instanceMethod myMethod (void) {
  String new s_start;
  Integer new i_start;
  OBJECT *i_alias;
  OBJECT *s_alias;
  

  s_alias = self;
  i_alias = i_start;
  i_start = 6;

  s_start = s_alias subString i_alias, 5;

  printf ("%s\n", s_start);
}

int main () {
  String new myString;

  myString = "StringValue";
  myString myMethod;
}
