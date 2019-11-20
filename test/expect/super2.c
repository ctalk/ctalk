/* Checks a prefix operator before "super." */


String instanceMethod isEven (Integer intArg) {
  returnObjectClass Boolean;

  if ((intArg % 2) == 0)
    return true;
  else
    return false;
}

String instanceMethod myMainMethod (void) {
  List new myList;
  myList = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10;
  myList map {
    /* Works already with innerparens, because the parens
       cause the parser not to include the ! with the expression;
       i.e., it's a C operator at the start of the expression. */
    /* if (!(super isEven self)) { */
    if (!super isEven self) {
      printf ("%d ", self);
    }
  }
  printf ("\n");
}


int main () {
  String new myString;
  myString myMainMethod;
}
