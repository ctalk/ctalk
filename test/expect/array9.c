
int main () {

  struct _my_struct {
    int mbr;
  } myStructArray[20] = { 0, };  /* make sure we can handle the */
                               /* initializer, too.           */
  int j;

  for (j = 0; j < 20; ++j) {
    myStructArray[j].mbr = 0;
  }

  myStructArray[10].mbr = 1;

  for (j = 1; j < 19; ++j) {
    /* this expression needs no translation to Ctalk. */
    if (myStructArray[j-1].mbr == 1 ||
	myStructArray[j].mbr == 1 ||
	myStructArray[j+1].mbr == 1) {
      printf ("%d\n", j);
    }
  } 
}
