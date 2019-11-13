
Integer class MyClass;
MyClass instanceVariable myInstanceVariable Integer 0;

Integer instanceMethod becomeMethod (void) {
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
    self become j;
    printf ("%d ", self);
  }
  printf ("\n");

  j0 = 0;
  self become j0;
  printf ("%d ", self);
  j1 = 1;
  self become j1;
  printf ("%d ", self);
  j2 = 2;
  self become j2;
  printf ("%d ", self);
  j3 = 3;
  self become j3;
  printf ("%d ", self);
  j4 = 4;
  self become j4;
  printf ("%d ", self);
  j5 = 5;
  self become j5;
  printf ("%d ", self);
  j6 = 6;
  self become j6;
  printf ("%d ", self);
  j7 = 7;
  self become j7;
  printf ("%d ", self);
  j8 = 8;
  self become j8;
  printf ("%d ", self);
  j9 = 9;
  self become j9;
  printf ("%d ", self);

  printf ("\n");
  return NULL;
}

int main () {
  MyClass new i;
  i myInstanceVariable becomeMethod;
}
