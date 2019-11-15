/* Check some parenthesization. */


List new globalList;

Application instanceMethod myListCheckMethod1 (void) {

  returnObjectClass Integer;

  Key new destListPtr;
  Integer new offset;

  destListPtr = *globalList;

  for (offset = 0; offset < 5; ++offset) {
    if ((*destListPtr) value == 1) {
      return offset;
    }
    destListPtr = globalList + offset;
  }

  return offset;
}

Application instanceMethod myListCheckMethod2 (void) {

  returnObjectClass Integer;

  Key new destListPtr;
  Integer new offset;

  destListPtr = *globalList;

  for (offset = 0; offset < 5; ++offset) {
    if (((*destListPtr) value) == 1) {
      return offset;
    }
    destListPtr = globalList + offset;
  }

  return offset;
}

int main () {

  Application new myApplication;
  Integer new i;

  globalList push 0;
  globalList push 0;
  globalList push 1;
  globalList push 0;
  globalList push 0;

  i = myApplication myListCheckMethod1;
  printf ("%d\n", i);
  
  globalList delete;

  globalList push 0;
  globalList push 0;
  globalList push 1;
  globalList push 0;
  globalList push 0;

  i = myApplication myListCheckMethod2;
  printf ("%d\n", i);
}
