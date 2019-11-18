
Integer instanceMethod myMethod (Integer myParam) {
  Integer new myOtherObject;
  List new myList;

  myOtherObject = 2;

  myList = "one";

  myList map {
    myParam = myOtherObject;

    if (myParam == myOtherObject)
      printf ("Pass\n");
    else
      printf ("Fail\n");
  }
}

int main () {
  Integer new myInt, myArg;

  myInt myMethod myArg;

  printf ("%d\n", myArg);
}
