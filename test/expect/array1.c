
/*
 *  Test array-to-C translation.
 */

int main () {

  Array new myArray;
  Integer new int1;
  Integer new int2;
  LongInteger new longInt1;
  LongInteger new longInt2;
  Character new char1;
  String new string1;

  int1 = 1;
  int2 = 2;

  myArray atPut 0, int1;
  myArray atPut 1, int2;

  printf ("%d %d ", (myArray at 0), (myArray at 1));

  longInt1 = 5ll;
  longInt2 = 6L;

  /*  This causes a warning when truncated to int. */
  /* myArray atPut 2, longInt1; */
  /* myArray atPut 3, longInt2; */

  /* printf ("%d %d ", (myArray at 2), (myArray at 3)); */

  char1 = 'c';

  myArray atPut 4, char1;

  printf ("%c ", myArray at 4);

  string1 = "s1";

  myArray atPut 5, string1;

  printf ("%s ", myArray at 5);

  printf ("\n");
}
