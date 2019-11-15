
int main () {

  int x;
  int z;
  int x_2;
  Integer new i;

  x = 10;
  i = 5;
  z = 14;
  x_2 = 6;

  if (x + ++i + ++x_2 < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  i = 5;
  x_2 = 6;

  if (x + --i + --x_2 < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  i = 5;
  x_2 = 6;

  if (x + ~i + x_2 > z) {  /* The ~ causes the total to be less. */
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + !i + x_2 < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + -i + x_2 > z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  /* The & and * operators are meaningless here, but
     they are handled similarly. sizeof is a different
     class of expression because it contains parentheses. 
  */
}
