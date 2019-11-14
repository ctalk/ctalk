/* $Id: clsvars1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Class variables that use a primitive constructor, no
 *  extra instance variables.  Test expressions in both
 *  receiver and C contexts.
 */

Event class MyClass;
MyClass classVariable classInt Integer 0;
MyClass classVariable classChar Character '';
MyClass classVariable classLongInt LongInteger 0LL;
MyClass classVariable classFloat Float 0.0;
MyClass classVariable classString String "";

int main () {

  MyClass new myClassObject;

#if 0
  MyClass classInt = 1;
  printf ("%d ", MyClass classInt);

  MyClass classInt = 2;
  printf ("%d ", MyClass classInt);

  MyClass classInt = 3;
  printf ("%d\n", MyClass classInt);

  myClassObject classInt = 1;
  printf ("%d ", myClassObject classInt);

  myClassObject classInt = 2;
  printf ("%d ", myClassObject classInt);

  myClassObject classInt = 3;
  printf ("%d\n", myClassObject classInt);

  classInt = 1;
  printf ("%d ", classInt);

  classInt = 2;
  printf ("%d ", classInt);

  classInt = 3;
  printf ("%d\n", classInt);

  MyClass classChar = 'a';
  printf ("%c ", MyClass classChar);

  MyClass classChar = 'b';
  printf ("%c ", MyClass classChar);

  MyClass classChar = 'c';
  printf ("%c\n", MyClass classChar);

  myClassObject classChar = 'a';
  printf ("%c ", myClassObject classChar);

  myClassObject classChar = 'b';
  printf ("%c ", myClassObject classChar);

  myClassObject classChar = 'c';
  printf ("%c\n", myClassObject classChar);

  classChar = 'a';
  printf ("%c ", classChar);

  classChar = 'b';
  printf ("%c ", classChar);

  classChar = 'c';
  printf ("%c\n", classChar);
#endif
  
  MyClass classLongInt = 1ll;
  printf ("%lldll ", MyClass classLongInt);

  MyClass classLongInt = 2ll;
  printf ("%lldll ", MyClass classLongInt);

  MyClass classLongInt = 3ll;
  printf ("%lldll\n", MyClass classLongInt);

  myClassObject classLongInt = 1ll;
  printf ("%lldll ", myClassObject classLongInt);

  myClassObject classLongInt = 2ll;
  printf ("%lldll ", myClassObject classLongInt);

  myClassObject classLongInt = 3ll;
  printf ("%lldll\n", myClassObject classLongInt);

  classLongInt = 1ll;
  printf ("%lldll ", classLongInt);

  classLongInt = 2ll;
  printf ("%lldll ", classLongInt);

  classLongInt = 3ll;
  printf ("%lldll\n", classLongInt);

  MyClass classFloat = 1.0;
  printf ("%f ", MyClass classFloat);

  MyClass classFloat = 2.0;
  printf ("%f ", MyClass classFloat);

  MyClass classFloat = 3.0;
  printf ("%f\n", MyClass classFloat);

  myClassObject classFloat = 1.0;
  printf ("%f ", myClassObject classFloat);

  myClassObject classFloat = 2.0;
  printf ("%f ", myClassObject classFloat);

  myClassObject classFloat = 3.0;
  printf ("%f\n", myClassObject classFloat);

  classFloat = 1.0;
  printf ("%f ", classFloat);

  classFloat = 2.0;
  printf ("%f ", classFloat);

  classFloat = 3.0;
  printf ("%f\n", classFloat);

  MyClass classString = "str1";
  printf ("%s ", MyClass classString);

  MyClass classString = "str2";
  printf ("%s ", MyClass classString);

  MyClass classString = "str3";
  printf ("%s\n", MyClass classString);

  myClassObject classString = "str1";
  printf ("%s ", myClassObject classString);

  myClassObject classString = "str2";
  printf ("%s ", myClassObject classString);

  myClassObject classString = "str3";
  printf ("%s\n", myClassObject classString);

  classString = "str1";
  printf ("%s ", classString);

  classString = "str2";
  printf ("%s ", classString);

  classString = "str3";
  printf ("%s\n", classString);

}

