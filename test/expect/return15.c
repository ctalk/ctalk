/* Checks the method parameter at the start of conditional expressions,
   and a parameter that shadows a function. */

Object class MyClass;

MyClass instanceMethod returnParam (String input, Integer index) {
  returnObjectClass Integer;
  return index;
}

int main () {

  MyClass new myObject;
  Integer new i;
  String new str;

  str = "Hello, world!\n";
  i = 6;
  
  printf ("%d\n", myObject returnParam str, i);
}
