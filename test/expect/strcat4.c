/* tests the cXstrcat template */

int main () {
  String new myStr;
  String new mySubStr1, mySubStr2;

  mySubStr1 = "Hello, ";
  mySubStr2 = "world!";

  myStr = xstrcat (mySubStr1, mySubStr2);

  printf ("%s\n", myStr);

}
