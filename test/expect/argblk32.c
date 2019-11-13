
Object class MyClass;
MyClass instanceVariable x Integer 0;

List new myList;

Application instanceMethod myMethod (void) {
  int r;
  Symbol new myObjPtr;
  int x;
  int x_val;
  
  *myObjPtr = MyClass basicNew "myClass 1", "(null)";
  (*myObjPtr) x = 1;
  myList push *myObjPtr;
  *myObjPtr = MyClass basicNew "myClass 2", "(null)";
  (*myObjPtr) x = 2;
  myList push *myObjPtr;
  *myObjPtr = MyClass basicNew "myClass 3", "(null)";
  (*myObjPtr) x = 3;
  myList push *myObjPtr;
  *myObjPtr = MyClass basicNew "myClass 4", "(null)";
  (*myObjPtr) x = 4;
  myList push *myObjPtr;
  *myObjPtr = MyClass basicNew "myClass 5", "(null)";
  (*myObjPtr) x = 5;
  myList push *myObjPtr;

  /* Check for a namespace collision between "x" the C variable and 
     "x" the instance variable. */
  myList map {
    x = self x;
    printf ("%d\n", x);
  }

}

int main () {

  Application new myApplication;
  myApplication myMethod;

}
