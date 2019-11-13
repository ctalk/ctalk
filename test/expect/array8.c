
int main () {

  Integer new myInt;

  struct _my_struct {
    int mbr;
  } myStructArray[20] = { 0, };  /* make sure we can handle the */
                               /* initializer, too.           */
  int j;

  myInt = 1;

  for (j = 0; j < 20; ++j) {
    myStructArray[j].mbr = 0;
  }

  myStructArray[10].mbr = myInt;

  for (j = 1; j < 19; ++j) {
    if (myStructArray[j-1].mbr == myInt ||
	myStructArray[j].mbr == myInt ||
	myStructArray[j+1].mbr == myInt) {
      printf ("%d\n", j);
    }
  } 
}
