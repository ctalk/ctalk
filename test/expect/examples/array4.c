Array instanceMethod printArrayElement (void) {

  WriteFileStream classInit;

  stdoutStream writeStream self;

  return NULL;
}

int main () {

  Array new myArray;

  myArray atPut 0, "My";
  myArray atPut 1, "name";
  myArray atPut 2, "is";
  myArray atPut 3, "Bill";

  myArray map printArrayElement;

  printf ("\n");

  myArray atPut 3, "Joe";

  myArray map printArrayElement;

  printf ("\n");
}
