/* checks some more class cast cases for objects. */

Application instanceMethod myMethod (void) {

  List new myList;
  Object new myObject;
  Key new myKey;
  Symbol new myIntPtr;
  Integer new myOffset;

  *myIntPtr = Integer basicNew "myInt1", "1";
  myList push *myIntPtr;
  *myIntPtr = Integer basicNew "myInt2", "2";
  myList push *myIntPtr;
  *myIntPtr = Integer basicNew "myInt3", "3";
  myList push *myIntPtr;

  myOffset = 2;

  myKey = myList + myOffset;

  myObject = *myKey;

  if ((Integer *)myObject == 3) {
    (Integer *)myObject = 4;
    printf ("%d\n", myObject value);
    printf ("-------\n");
  }    

  /* Again, the assignment doesn't actually change the value in the
     list, only the object that myObject refers to. */
  myList map {
    printf ("%d\n", self value);
  }

}

int main () {

  Application new myApplication;

  myApplication myMethod;
  
}
