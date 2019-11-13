
struct _my_struct {
  int mbr;
} myStructArray[20] = { 0, };  /* make sure we can handle the */
                               /* initializer, too.           */
struct _my_struct *myStructPtrs[20];

int main () {

  Integer new myInt;

  int j;

  myInt = 1;

  for (j = 0; j < 20; ++j) {
    myStructPtrs[j] = (struct _my_struct *)malloc (sizeof (struct _my_struct));
    myStructPtrs[j] -> mbr = 0;
  }

  myStructPtrs[10] -> mbr = myInt;

  for (j = 1; j < 19; ++j) {
    if (myStructPtrs[j-1] -> mbr == myInt ||
	myStructPtrs[j] -> mbr == myInt ||
	myStructPtrs[j+1] -> mbr == myInt) {
      printf ("%d\n", j);
    }
  } 
  for (j = 0; j < 20; ++j) {
    free (myStructPtrs[j]);
  }
}
