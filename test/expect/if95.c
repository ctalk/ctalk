
int main () {

  int x;
  Point new pt;

  x = 10;
  pt y = 5;
  

  if (x + ++pt y < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  pt y = 5;

  if (x + --pt y < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + ~pt y > 14) {  /* the ~ operator causes the term to be negative */
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + !pt y > 14) { 
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + -pt y > 14) { 
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }
  
}
