/* myListIncMethod uses a class cast to assign an Integer by reference - 
 that is, to assign the Integer using Object : = (which assigns by
 reference) instead of Integer : = (which assigns by value). */

List new globalList;

Application instanceMethod myListIncMethod (void) {

  Integer new memberInt;

  globalList map {
    (Object *)memberInt = self;
    memberInt += 5;
  }
}

int main () {

  Application new myApplication;

  globalList push 1;
  globalList push 2;
  globalList push 3;
  globalList push 4;
  globalList push 5;

  globalList map {
    printf ("%d\n", self value);
  }

  myApplication myListIncMethod;
  
  printf ("---------------------\n");

  globalList map {
    printf ("%d\n", self value);
  }
}
