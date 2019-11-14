void myfunc (float a, float b) {
  printf ("%lf %lf\n", a, b);
}

Integer instanceMethod myMethod (Integer x, Integer y) {

  /* Tests the cast evaluation here..... */
  myfunc (5.0, 5.0 * ((float)x / (float)y));

}

int main () {

  Integer new myInt;

  myInt myMethod 11, 14;
}
