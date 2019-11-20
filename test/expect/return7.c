Array instanceMethod myValue (void) {
  char a[10];
  a[0] = 'a';
  a[1] = 'b';
  a[2] = 'c';
  a[3] = 'd';
  a[4] = 'e';
  a[5] = 'f';
  a[6] = 'g';
  a[7] = 'h';
  a[8] = 'i';
  a[9] = 'j';
  return a;
}


int main () {
  Array new a;
  a myValue map {
    printf ("%c\n", self);
  }
}
