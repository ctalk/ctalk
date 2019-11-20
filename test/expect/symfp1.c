
void my_func (void) {
  printf ("Printed by my_func.\n");
}

int main (int argc, char **argv) {
  Symbol new fnSym;
  OBJECT *sym_alias;
  void (*fn_alias) ();

  fnSym = my_func;

  sym_alias = fnSym value;

  /* fn_alias = (void *)strtol (sym_alias -> __o_value, NULL, 16);*//***/
  fn_alias = *(void **)sym_alias -> __o_value;
  (*fn_alias)();
}
