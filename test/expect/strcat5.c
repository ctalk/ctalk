
int main () {
  String new mySubStr1, mySubStr2;
  Integer new len;

  mySubStr1 = "Hello, ";
  mySubStr2 = "world!";

  xstrcat (mySubStr1, mySubStr2);

  printf ("%s\n", mySubStr1);

  mySubStr2 = " and world and world!";

  xstrcat (mySubStr1, mySubStr2);

  printf ("%s\n", mySubStr1);

}
