
int main () {
  Character new myChar;

  myChar = 'a';

  if (myChar < 'b') {
    printf ("a < 'b' -> True: Success.\n");
  } else {
    printf ("a < 'b': -> False: Fail.\n");
  }

  if (myChar > 'b') {
    printf ("a > 'b' -> True: Fail.\n");
  } else {
    printf ("a > 'b' -> False : Success.\n");
  }

  if (myChar < 98) {
    printf ("a < 98 -> True: Success.\n");
  } else {
    printf ("a < 98 -> False: Fail.\n");
  }

  if (myChar > 98) {
    printf ("a > 98 -> True: Fail.\n");
  } else {
    printf ("a > 98 -> False: Success.\n");
  }

  if (myChar < 98l) {
    printf ("a < 98l -> True: Success.\n");
  } else {
    printf ("a > 98l -> False: Fail.\n");
  }

  if (myChar > 98l) {
    printf ("a > 98l -> True: Fail.\n");
  } else {
    printf ("a > 98l -> False: Success.\n");
  }

  if (myChar < 98ll) {
    printf ("a < 98ll -> True: Success.\n");
  } else {
    printf ("a < 98ll -> False: Fail.\n");
  }

  if (myChar > 98ll) {
    printf ("a > 98ll -> True: Fail.\n");
  } else {
    printf ("a > 98ll -> False: Success.\n");
  }

  if (myChar < 98L) {
    printf ("a < 98L -> True: Success.\n");
  } else {
    printf ("a < 98L -> False: Fail.\n");
  }

  if (myChar > 98L) {
    printf ("a > 98L -> True: Fail.\n");
  } else {
    printf ("a > 98L -> False: Success.\n");
  }

  if (myChar < 98.0) {
    printf ("a < 98.0 -> True: Success.\n");
  } else {
    printf ("a < 98.0 -> False: Fail.\n");
  }

  if (myChar > 98.0) {
    printf ("a > 98.0 -> True: Fail.\n");
  } else {
    printf ("a > 98.0 -> False: Success.\n");
  }

}
