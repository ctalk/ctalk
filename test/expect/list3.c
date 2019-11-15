
List new globalList;

Application instanceMethod myListAssignMethod (List dest_list) {

  Integer new memberInt;
  Key new destListPtr;
  Symbol new memberIntPtr;

  destListPtr = *dest_list;
  globalList map {
    *memberIntPtr  = *destListPtr;
    (*memberIntPtr) value = self value;
    ++destListPtr;
  }
}

int main () {

  Application new myApplication;
  List new localList;
  Integer new i;

  localList push 10;
  localList push 11;
  localList push 12;
  localList push 13;
  localList push 14;

  for (i = 1; i <= 10; ++i) {
    printf ("i = %d\n", i);
    globalList push 1 * i;
    globalList push 2 * i;
    globalList push 3 * i;
    globalList push 4 * i;
    globalList push 5 * i;

    globalList map {
      printf ("%d ", self value);
    }

    myApplication myListAssignMethod localList;
  
    printf ("\n---------------------\n");

    localList map {
      printf ("%d ", self value);
    }

    printf ("\n");
   
    globalList delete;
  }
}
