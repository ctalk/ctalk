LongInteger instanceMethod bitCompOverload (void) {
  LongInteger new i2;
  i2 = ~self value;
  printf ("%d\n", i2);
  printf ("%d\n", ~self);
  i2 = self value;
  printf ("%d\n", ~i2);
}

int main () {
  LongInteger new i;
  i = 1L;
  printf ("The following numbers should be the same.\n");
  printf ("%d\n", ~i);
  i bitCompOverload;
}
