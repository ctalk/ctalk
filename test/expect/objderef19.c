
int main () {

  String new s;
  OBJECT *s_alias;
  Integer new scopeInt;
  

  s_alias = s;

  scopeInt = s -> scope;

  printf ("%d == %d == %d\n", s -> scope, s_alias -> scope, scopeInt);

}
