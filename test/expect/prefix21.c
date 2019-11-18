Character instanceMethod decOverload (void) {
  Character new c2;
  c2 = --self value;
  printf ("%c\n", c2);
  printf ("%c\n", --self);
  printf ("%c\n", --c2);
}

int main () {
  Character new c;
  c = 'z';
  printf ("%c\n", --c);
  c decOverload;
}
