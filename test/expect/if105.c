

Point instanceMethod ifEq (void) {
  int x;
  Integer new x_2;

  x = 16;
  x_2 = 16;

  if (x + x_2 + ++self y asCharacter == 113) {   /* 'p' + 1 */
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (x + x_2 + --self y asCharacter == 111) {   /* 'o' */
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (x + x_2 + ~self y asCharacter == 31) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (x + x_2 + !self y asCharacter == 32) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (x + x_2 + -self y asCharacter == -48) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

}

int main () {

  Point new pt;

  pt y = 80;   /* 'P' */

  pt ifEq;
}
