/* $Id: cmplmaa2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Class variables that use a primitive constructor.
 */

Event class MyClass;
MyClass classVariable classInteger Integer 0;
MyClass classVariable classCharacter Character '';
MyClass classVariable classString String "";
MyClass classVariable classLongInteger LongInteger 0L;
MyClass classVariable classFloat Float 0.0;

int main () {

  MyClass classCharacter = 'a';
  printf ("%c ", MyClass classCharacter);
  MyClass classCharacter = 'b';
  printf ("%c ", MyClass classCharacter);
  MyClass classCharacter = 'c';
  printf ("%c\n", MyClass classCharacter);

  MyClass classString = "string1";
  printf ("%s ", MyClass classString);
  MyClass classString = "string2";
  printf ("%s ", MyClass classString);
  MyClass classString = "string3";
  printf ("%s\n", MyClass classString);

  MyClass classInteger = 1;
  printf ("%d ", MyClass classInteger);
  MyClass classInteger = 2;
  printf ("%d ", MyClass classInteger);
  MyClass classInteger = 3;
  printf ("%d\n", MyClass classInteger);

  MyClass classLongInteger = 1ll;
  printf ("%lld ", MyClass classLongInteger);
  MyClass classLongInteger = 2ll;
  printf ("%lld ", MyClass classLongInteger);
  MyClass classLongInteger = 3ll;
  printf ("%lld\n", MyClass classLongInteger);

  MyClass classFloat = 1.0;
  printf ("%f ", MyClass classFloat);
  MyClass classFloat = 2.0;
  printf ("%f ", MyClass classFloat);
  MyClass classFloat = 3.0;
  printf ("%f\n", MyClass classFloat);

  exit(0);
}
