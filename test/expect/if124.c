

Point instanceMethod ifEq (void) {
  int x;
  Integer new x_2;
  List new l;

  x = 16;
  x_2 = 16;
  l = "one";

  l map {
    /* The postfix operators should generate a warning and have 
       no effect. */
    if (x + super y asCharacter++ + x_2 == 112) {   /* 'p' */
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }

    if (x + super y asCharacter-- + x_2 == 112) {   /* 'p' */
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  }

}

int main () {

  Point new pt;

  pt y = 80;   /* 'P' */

  pt ifEq;
}
