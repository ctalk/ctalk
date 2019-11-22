int main () {
  Array new myArray;

  myArray atPut 0, "My";
  myArray atPut 1, "name";
  myArray atPut 2, "is";
  myArray atPut 3, "Bill";

  WriteFileStream classInit;

  stdoutStream printOn "%s %s %s %s.\n", myArray at 0, 
    myArray at 1, myArray at 2, myArray at 3;

  myArray atPut 3, "Joe";

  stdoutStream printOn "%s %s %s %s.\n", myArray at 0, myArray at 1, 
    myArray at 2, myArray at 3;
}
