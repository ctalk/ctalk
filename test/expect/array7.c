
int myArray[20][20];
Integer new myInt;

int main () {
  int i, j;

  myInt = 1;

  for (j = 0; j < 20; ++j) {
    for (i = 0; i < 20; ++i) {
      myArray[j][i] = 0;
    }
  }

  myArray[10][10] = myInt;

  for (j = 1; j < 19; ++j) {
    for (i = 1; i < 19; ++i) {
      if (myArray[j-1][i] == myInt ||
	  myArray[j][i] == myInt ||
	  myArray[j+1][i] == myInt) {
	printf ("%d, %d\n", i, j);
      }
    }
  } 
}
