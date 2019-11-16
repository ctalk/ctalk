/* This checks that the method, "contains," (in the for loop)
   can recognize that its receiver is the result of the method,
   "at." */


int main () {
  String new s;
  Array new a;
  Integer new i;
  String new searchString;

  s = "2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36";
  searchString = "2";

  s split ' ', a;

  for (i = 0; i < a size; i++) {
    if (a at i contains searchString) {
      printf ("%s\n", a at i);
    }
  }
}
