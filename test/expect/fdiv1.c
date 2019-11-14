
/*
 *  Tests only for validity against libm's floating point
 *  math.  
 */

int main (int argc, char **argv) {
  Float new f1;
  Float new f2;
  Float new f3;
  Float new f4;
  Float new f5;
  Float new fResult;

  double d1;
  double d2;
  double d3;
  double d4;
  double d5;
  double dResult;
  
  f1 = 1.0;
  f2 = 2.0;
  f3 = 3.0;
  f4 = 4.0;
  f5 = 5.0;

  d1 = 1.0;
  d2 = 2.0;
  d3 = 3.0;
  d4 = 4.0;
  d5 = 5.0;

  /* 0.16666667 */
  fResult = f1 / f2 / f3;
  dResult = d1 / d2 / d3;

  if (fResult == dResult) {
    printf ("%lf == %lf -- Pass\n", fResult, dResult);
  } else {
    printf ("%lf != %lf -- Fail\n", fResult, dResult);
  }

  /* 0.16666667 */
  fResult = (f1 / f2) / f3;
  dResult = (d1 / d2) / d3;

  if (fResult == dResult) {
    printf ("%lf == %lf -- Pass\n", fResult, dResult);
  } else {
    printf ("%lf != %lf -- Fail\n", fResult, dResult);
  }

  fResult = ((f1 / f2) / f3) / f4;
  dResult = ((d1 / d2) / d3) / d4;

  if (fResult == dResult) {
    printf ("%lf == %lf -- Pass\n", fResult, dResult);
  } else {
    printf ("%lf != %lf -- Fail\n", fResult, dResult);
  }

  fResult = (((f1 / f2) / f3) / f4) / f5;
  dResult = (((d1 / d2) / d3) / d4) / d5;

  if (fResult == dResult) {
    printf ("%lf == %lf -- Pass\n", fResult, dResult);
  } else {
    printf ("%lf != %lf -- Fail\n", fResult, dResult);
  }

  fResult = ((f1 / f2) / f3) / f4;
  dResult = ((d1 / d2) / d3) / d4;

  if (fResult == dResult) {
    printf ("%lf == %lf -- Pass\n", fResult, dResult);
  } else {
    printf ("%lf != %lf -- Fail\n", fResult, dResult);
  }

  fResult = ((f1 / f1) / f2) / f3;
  dResult = ((d1 / d1) / d2) / d3;

  if (fResult == dResult) {
    printf ("%lf == %lf -- Pass\n", fResult, dResult);
  } else {
    printf ("%lf != %lf -- Fail\n", fResult, dResult);
  }

  fResult = (f1 / (f2 / f2)) / f3;
  dResult = (d1 / (d2 / d2)) / d3;

  if (fResult == dResult) {
    printf ("%lf == %lf -- Pass\n", fResult, dResult);
  } else {
    printf ("%lf != %lf -- Fail\n", fResult, dResult);
  }

  fResult = (f1 / f2) / (f3 / f3);
  dResult = (d1 / d2) / (d3 / d3);

  if (fResult == dResult) {
    printf ("%lf == %lf -- Pass\n", fResult, dResult);
  } else {
    printf ("%lf != %lf -- Fail\n", fResult, dResult);
  }

}
