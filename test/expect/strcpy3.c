/* Tests the cXstrcpy template */

int main () {
  String new myStr;
  String new mySubStr1, mySubStr2;

  mySubStr1 = "Hello, ";
  mySubStr2 = "world!";

  myStr = xstrcpy (mySubStr1, mySubStr2);

  printf ("%s\n", myStr);

}
