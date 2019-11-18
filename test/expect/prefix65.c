/* Checks the method parameter at the start of conditional expressions,
   and a parameter that shadows a function. */

Object class MyClass;

MyClass instanceMethod newline (String input, int index) {
  while (input at index) {
    if (input at index == '\n') {
      return index;
    } else {
      ++index;
    }
  }
  return 0;
}

int main () {

  MyClass new myObject;
  Integer new i;
  String new str;

  str = "Hello,\nworld!";
  i = 0;
  
  printf ("%d\n", myObject newline str, i);
}
