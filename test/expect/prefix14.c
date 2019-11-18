Integer instanceMethod bitCompOverload (void) {
  Integer new i2;
  i2 = ~self value;
  printf ("%d\n", i2);
  printf ("%d\n", ~self);
  i2 = self value;
  printf ("%d\n", ~i2);
}

int main () {
  Integer new i;
  i = 1;
  printf ("The following numbers should be the same.\n");
  printf ("%d\n", ~i);
  i bitCompOverload;
}
