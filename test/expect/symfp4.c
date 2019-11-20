

void myfunc (void) {
  printf ("hi!\n");
}

Integer instanceMethod myMethod (Integer i, Symbol fn, Integer v) {
  OBJECT *sym_alias;
  void (*fn_alias)();
  sym_alias = fn value;
  /* fn_alias = (void *)strtol (sym_alias -> __o_value, NULL, 16); *//***/
  fn_alias = *(void **)sym_alias -> __o_value;
  (*fn_alias)();
  
}

int main () {
  Integer new i;

  i myMethod 1, myfunc, 0;
}
