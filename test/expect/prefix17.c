Integer instanceMethod decOverload (void) {
  Integer new i2;
  i2 = --self value;
  printf ("%d\n", i2);
  printf ("%d\n", --self);
  printf ("%d\n", --i2);
}

int main () {
  Integer new i;
  i = 5;
  printf ("%d\n", --i);
  i decOverload;
}
