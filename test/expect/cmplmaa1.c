/* $Id: cmplmaa1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

Event class MyClass;
MyClass instanceVariable myArray AssociativeArray NULL;

MyClass instanceMethod new (char *myClassObjectName) {
  MyClass super new myClassObjectName;

  __ctalkInstanceVarsFromClassObject (myClassObjectName);

  return myClassObjectName;
}

int main () {

  int i;
  MyClass new myTestObject;

  myTestObject myArray atPut "key1", "value_without_parens_around_args";
  printf ("%s\n", myTestObject myArray at "key1");

  myTestObject myArray atPut ("key2", "value_with_parens_around_arglists");
  printf ("%s\n", myTestObject myArray at ("key2"));

  myTestObject myArray atPut ("key3"), ("value_with_parens_around_args");
  printf ("%s\n", myTestObject myArray at ("key3"));

  exit(0);
}
