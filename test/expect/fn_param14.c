void myfunc2 (float a, float b, float c) {
  printf ("%lf %lf\n", a, b);
}

void myfunc (float a, float b) {
  printf ("%lf %lf\n", a, b);
}

Integer instanceMethod myMethod (Integer x, Integer y) {

  /* Tests the cast evaluations here..... */
  myfunc2 (5.0, 5.0 * (float)x / (float)y, 3.0);
  myfunc (5.0, 5.0 * (float)x / (float)y);

}

int main () {

  Integer new myInt;

  myInt myMethod 11, 14;
}
