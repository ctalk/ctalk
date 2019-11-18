


/* Test the sequences of operators that the compiler doesn't issue an
 *  error or warning for. 
 */

int main () {
  Integer new i;
  i = 0;
  if (!i) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (!!i) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (~!!i) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

#if 0
  /* This is now an operator... */
  if (!~!i) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }
#endif
}
