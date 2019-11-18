/*  tests the leading paren and rvalue object translation to the
    C lvalue in prefix_method_expr_a. */

Object class  MyClass;
MyClass instanceVariable myFloat1 Float 0.0f;
MyClass instanceVariable myFloat2 Float 0.0f;
MyClass instanceVariable myFloat3 Float 00.0f;

List new myList;

Application instanceMethod myMethod (Integer param_int) {
  Key new k;
  float f1, f2, f3;

  k = myList + param_int;

  f1 = (*k) myFloat1;
  f2 = (*k) myFloat2;
  f3 = (*k) myFloat3;

  printf ("%f %f %f\n", f1, f2, f3);
}

int main () {
  Symbol new listPtr;
  Application new myApp;

  *listPtr = MyClass basicNew "item1";
  (*listPtr) myFloat1 = 0.0f;
  (*listPtr) myFloat2 = 1.0f;
  (*listPtr) myFloat3 = 2.0f;
  myList push *listPtr;
  *listPtr = MyClass basicNew "item2";
  (*listPtr) myFloat1 = 3.0f;
  (*listPtr) myFloat2 = 4.0f;
  (*listPtr) myFloat3 = 5.0f;
  myList push *listPtr;
  *listPtr = MyClass basicNew "item3";
  (*listPtr) myFloat1 = 6.0f;
  (*listPtr) myFloat2 = 7.0f;
  (*listPtr) myFloat3 = 8.0f;
  myList push *listPtr;

  myApp myMethod 1;
}
