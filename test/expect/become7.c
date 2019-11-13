
Integer class MyClass;
MyClass instanceVariable myInstanceVariable Integer 0;

Integer instanceMethod doBecome (Integer i) {
  self become i;
  return self;
}

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
/*     self become j; */
    self doBecome j;
    printf ("%d ", __ctalkToCInteger(__ctalk_self_internal (), 1));
  }
  printf ("\n");

  j0 = 0;
  self doBecome j0;
  printf ("%d ", self);
  j1 = 1;
  self doBecome j1;
  printf ("%d ", self);
  j2 = 2;
  self doBecome j2;
  printf ("%d ", self);
  j3 = 3;
  self doBecome j3;
  printf ("%d ", self);
  j4 = 4;
  self doBecome j4;
  printf ("%d ", self);
  j5 = 5;
  self doBecome j5;
  printf ("%d ", self);
  j6 = 6;
  self doBecome j6;
  printf ("%d ", self);
  j7 = 7;
  self doBecome j7;
  printf ("%d ", self);
  j8 = 8;
  self doBecome j8;
  printf ("%d ", self);
  j9 = 9;
  self doBecome j9;
  printf ("%d ", self);

  printf ("\n");
  return NULL;
}

int main () {
  MyClass new i;
  i myInstanceVariable becomeMethod;
}
