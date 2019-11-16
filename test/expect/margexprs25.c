/* $Id: margexprs25.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main () {

  Integer new myInt;
  LongInteger new myLongInt;
  Float new myFloat;

  Integer new myIntNegated;
  LongInteger new myLongIntNegated;
  Float new myFloatNegated;

  myInt = 25;
  myLongInt = 25l;
  myFloat = 25.0;

  printf ("%d\n", -myInt);
  printf ("%lld\n", -myLongInt);
  printf ("%f\n", -myFloat);

  myIntNegated = -myInt;
  myLongIntNegated = -myLongInt;
  myFloatNegated = -myFloat;

  printf ("%d\n", myIntNegated);
  printf ("%lld\n", myLongIntNegated);
  printf ("%f\n", myFloatNegated);

  exit(0);
}
