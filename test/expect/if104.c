

Point instanceMethod ifEq (void) {
  int x;
  Integer new x_2;

  x = 16;
  x_2 = 16;

  if (x + ++self y asCharacter + x_2 == 113) {   /* 'p' + 1 */
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (x + --self y asCharacter + x_2 == 111) {   /* 'o' */
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (x + ~self y asCharacter + x_2 == 31) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (x + !self y asCharacter + x_2 == 32) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (x + -self y asCharacter + x_2 == -48) {
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
