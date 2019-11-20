Array instanceMethod myValue (void) {
  int a[10];
  a[0] = 30;
  a[1] = 31;
  a[2] = 32;
  a[3] = 33;
  a[4] = 34;
  a[5] = 35;
  a[6] = 36;
  a[7] = 37;
  a[8] = 38;
  a[9] = 39;
  return a;
}


int main () {
  Array new a;
  a myValue map {
    printf ("%d\n", self);
  }
}
