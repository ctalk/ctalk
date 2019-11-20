int main () {
  Symbol new mySym;
  Integer new myInt;

  myInt = 1;

  /* See if we can make two warnings here, one for each assignment, 
     if we use the following statement. */
  /* mySym = myInt; */

  *mySym = myInt;
  printf ("%d\n", mySym getValue value);

  mySym = myInt asSymbol;
  printf ("%d\n", mySym getValue value);
  
  printf ("%d\n", mySym getValue getValue value);
}
