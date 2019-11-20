
void my_func (void) {
  printf ("Printed by my_func.\n");
}

Symbol instanceMethod setFn (Symbol fn) {
  Symbol new fnSym;

  fnSym = fn;

  eval ((*fnSym)());
  eval (*fnSym)();
  (*fnSym)();

}

int main (int argc, char **argv) {
  Symbol new s;

  s setFn my_func;
}
