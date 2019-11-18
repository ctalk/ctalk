/* Tests the cXstrcpy template */

int main () {
  String new mySubStr1, mySubStr2;

  mySubStr1 = "Hello, ";
  mySubStr2 = "world!";

  xstrcpy (mySubStr1, mySubStr2);

  printf ("%s\n", mySubStr1);

}
