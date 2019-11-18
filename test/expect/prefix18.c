LongInteger instanceMethod incOverload (void) {
  LongInteger new l2;
  l2 = ++self value;
  printf ("%d\n", l2);
  printf ("%d\n", ++self);
  printf ("%d\n", ++l2);
}

int main () {
  LongInteger new l;
  l = 5L;
  printf ("%d\n", ++l);
  l incOverload;
}
