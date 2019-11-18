Integer instanceMethod incOverload (void) {
  Integer new i2;
  i2 = ++self value;
  printf ("%d\n", i2);
  printf ("%d\n", ++self);
  printf ("%d\n", ++i2);
}

int main () {
  Integer new i;
  i = 1;
  printf ("%d\n", ++i);
  i incOverload;
}
