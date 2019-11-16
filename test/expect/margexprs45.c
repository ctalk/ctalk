
/* Check that a method parameter works in the if () clause with a
   prefix operator. */

Integer instanceMethod checkArg (Key k) {
  returnObjectClass Boolean;

  if (*k == '\0')
    return TRUE;
  else
    return FALSE;
}

int main () {

  Integer new myInt;
  Key new k;
  
  *k = "";
  
  if (myInt checkArg k) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

}
