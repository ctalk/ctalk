
int main () {
  Character new myChar;

  myChar = 'a';

  if (myChar == 'a') {
    printf ("a == a: Success.\n");
  } else {
    printf ("a == a: Fail.\n");
  }

  if (myChar == 97) {
    printf ("a == 97: Success.\n");
  } else {
    printf ("a == 97: Fail.\n");
  }

  if (myChar == 97l) {
    printf ("a == 97l: Success.\n");
  } else {
    printf ("a == 97l: Fail.\n");
  }

  if (myChar == 97ll) {
    printf ("a == 97ll: Success.\n");
  } else {
    printf ("a == 97ll: Fail.\n");
  }

  if (myChar == 97L) {
    printf ("a == 97L: Success.\n");
  } else {
    printf ("a == 97L: Fail.\n");
  }

  if (myChar == 97.0) {
    printf ("a == 97.0: Success.\n");
  } else {
    printf ("a == 97.0: Fail.\n");
  }

  if (myChar != 'b') {
    printf ("a != b: Success.\n");
  } else {
    printf ("a != b: Fail.\n");
  }

  if (myChar != 98) {
    printf ("a != 98: Success.\n");
  } else {
    printf ("a != 98: Fail.\n");
  }

  if (myChar != 98l) {
    printf ("a != 98l: Success.\n");
  } else {
    printf ("a 1= 98l: Fail.\n");
  }

  if (myChar != 98ll) {
    printf ("a != 98ll: Success.\n");
  } else {
    printf ("a != 98ll: Fail.\n");
  }

  if (myChar != 98L) {
    printf ("a != 98L: Success.\n");
  } else {
    printf ("a != 98L: Fail.\n");
  }

  if (myChar != 98.0) {
    printf ("a != 98.0: Success.\n");
  } else {
    printf ("a != 98.0: Fail.\n");
  }

}
