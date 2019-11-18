

/* this is here mostly so we can check there's a warning message. 
 in this case, the operator does affect the receiver, 'tho. */

int main () {
  Integer new myInt;

  myInt = 'c';
  myInt asInteger++;
  printf ("%c\n", (char)myInt);
}
