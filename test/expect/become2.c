
Integer instanceMethod becomeMethod (void) {
  Integer new i;
  Integer new j;
  Integer new j0;
  Integer new j1;
  Integer new j2;
  Integer new j3;
  Integer new j4;
  Integer new j5;
  Integer new j6;
  Integer new j7;
  Integer new j8;
  Integer new j9;
  Integer new cnt;

  for (cnt = 0; cnt < 10; cnt = cnt + 1) {
    j = cnt;
    i become j;
    printf ("%d ", i);
  }
  printf ("\n");

  j0 = 0;
  i become j0;
  printf ("%d ", i);
  j1 = 1;
  i become j1;
  printf ("%d ", i);
  j2 = 2;
  i become j2;
  printf ("%d ", i);
  j3 = 3;
  i become j3;
  printf ("%d ", i);
  j4 = 4;
  i become j4;
  printf ("%d ", i);
  j5 = 5;
  i become j5;
  printf ("%d ", i);
  j6 = 6;
  i become j6;
  printf ("%d ", i);
  j7 = 7;
  i become j7;
  printf ("%d ", i);
  j8 = 8;
  i become j8;
  printf ("%d ", i);
  j9 = 9;
  i become j9;
  printf ("%d ", i);

  printf ("\n");
}

int main () {
  Integer new i;
  i becomeMethod;
}
