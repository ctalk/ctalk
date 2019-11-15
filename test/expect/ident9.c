
Integer instanceMethod myMethod (Object myObject) {

  List new myList;
  OBJECT *a = NULL, *b = NULL;

#if 0
  if (a == NULL) {
    printf ("Pass\n");
  }
#endif
  
  a = __ctalkCreateObjectInit ("myObjectPtrA", "Integer",
			       "Magnitude", LOCAL_VAR,
			       "1");
  __ctalkInstanceVarsFromClassObject (a);
  __ctalkAddBasicNewTag (a);
  
  myList = "one";

  myList map {

    if (a == b) {
      printf ("Fail\n");
    } else {
      printf ("Pass\n");
    }

    b = a;

    if (b == a) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }

    if (myObject == NULL) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    
    myObject = a;

    if (myObject == a) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  
    myObject = a;

    if (a == myObject) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }

    b = a;

    if (myObject == b) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  
    if (b == myObject) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  }

}

int main () {
  Integer new myInt;
  Object new myObject;
  myInt myMethod myObject;
}
