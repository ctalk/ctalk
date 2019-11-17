
int main () {
  String new myStr;
  String new mySubStr1, mySubStr2;
  Integer new len;

  mySubStr1 = "Hello, ";
  mySubStr2 = "world!";
  /* len = strlen (mySubStr2) - 1; *//* not yet supported */
  len = strlen (mySubStr2);
  len -= 1;

  myStr = xstrncat (mySubStr1, mySubStr2, len);

  printf ("%s\n", myStr);

  mySubStr2 = " and world and world!";
  len = strlen (mySubStr2);

  myStr = xstrncat (mySubStr1, mySubStr2, len);

  printf ("%s\n", myStr);

}
