
void my_func (void) {
  printf ("Printed by my_func.\n");
}

Symbol instanceMethod setFn (Symbol fn) {
  Symbol new fnSym;
  OBJECT *sym_alias;
  void (*fn_alias) ();

  fnSym = fn;

  sym_alias = fnSym value;

  /* fn_alias = (void *)strtol (sym_alias -> __o_value, NULL, 16);*//***/
  fn_alias = *(void **)sym_alias -> __o_value;
  (*fn_alias)();
}

int main (int argc, char **argv) {
  Symbol new s;

  s setFn my_func;
}
