String instanceMethod printSelf (void) {
  printf ("%s\n", self);
}

int main () {
  char *a[5];
  int i;
  a[0] = strdup ("zero");
  a[1] = strdup ("one");
  a[2] = strdup ("two");
  a[3] = strdup ("three");
  a[4] = strdup ("four");

  for (i = 0; i < 5; ++i) {
    a[i] printSelf;
  }

  free (a[0]);
  free (a[1]);
  free (a[2]);
  free (a[3]);
  free (a[4]);
}
