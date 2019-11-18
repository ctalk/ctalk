Integer instanceMethod myAddr (void) {
  OBJECT *i_alias, *i_alias2, *i_alias3;
  Integer new i;
  Symbol new s;
  /* NOTE: This should translate to an Integer. */
  i_alias = i;
  /* This should translate to a Symbol. */
  i_alias2 = &i;
  *s = i;

  printf ("The following five addresses should be the same.\n");
  printf ("%p\n", i_alias);
  /* TODO - This could be a single expression, if we get to
     strange cases like these. */
  i_alias3 = *((OBJECT **)i_alias2 -> instancevars -> __o_value);
  printf ("%s\n", eval i_alias3 addressOf asAddrString);
  /*printf ("%s\n", (*(OBJECT **)i_alias2 -> instancevars ->__o_value) asString);*/
  printf ("%p\n", (s getValue) addressOf);
  printf ("%p\n", s getValue addressOf);
  printf ("%p\n", &i);
}

int main () {
  Integer new i;
  i myAddr;
}
