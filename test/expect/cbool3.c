
int main () {
  Character new myChar;

  myChar = 'a';

  if (myChar <= 'a') {
    printf ("a <= 'a' -> True: Success.\n");
  } else {
    printf ("a <= 'a': -> False: Fail.\n");
  }

  if (myChar <= 'b') {
    printf ("a <= 'b' -> True: Success.\n");
  } else {
    printf ("a <= 'b': -> False: Fail.\n");
  }

  if (myChar >= 'b') {
    printf ("a >= 'b' -> True: Fail.\n");
  } else {
    printf ("a >= 'b' -> False : Success.\n");
  }

  if (myChar <= 97) {
    printf ("a <= 97 -> True: Success.\n");
  } else {
    printf ("a <= 97 -> False: Fail.\n");
  }

  if (myChar <= 98) {
    printf ("a <= 98 -> True: Success.\n");
  } else {
    printf ("a <= 98 -> False: Fail.\n");
  }

  if (myChar >= 98) {
    printf ("a >= 98 -> True: Fail.\n");
  } else {
    printf ("a >= 98 -> False: Success.\n");
  }

  if (myChar <= 97l) {
    printf ("a <= 97l -> True: Success.\n");
  } else {
    printf ("a <= 97l -> False: Fail.\n");
  }

  if (myChar <= 98l) {
    printf ("a <= 98l -> True: Success.\n");
  } else {
    printf ("a <= 98l -> False: Fail.\n");
  }

  if (myChar >= 98l) {
    printf ("a >= 98l -> True: Fail.\n");
  } else {
    printf ("a >= 98l -> False: Success.\n");
  }

  if (myChar <= 97ll) {
    printf ("a <= 97ll -> True: Success.\n");
  } else {
    printf ("a <= 97ll -> False: Fail.\n");
  }

  if (myChar <= 98ll) {
    printf ("a <= 98ll -> True: Success.\n");
  } else {
    printf ("a <= 98ll -> False: Fail.\n");
  }

  if (myChar >= 98ll) {
    printf ("a >= 98ll -> True: Fail.\n");
  } else {
    printf ("a >= 98ll -> False: Success.\n");
  }

  if (myChar <= 97L) {
    printf ("a <= 97L -> True: Success.\n");
  } else {
    printf ("a <= 97L -> False: Fail.\n");
  }

  if (myChar <= 98L) {
    printf ("a <= 98L -> True: Success.\n");
  } else {
    printf ("a <= 98L -> False: Fail.\n");
  }

  if (myChar >= 98L) {
    printf ("a >= 98L -> True: Fail.\n");
  } else {
    printf ("a >= 98L -> False: Success.\n");
  }

  if (myChar <= 97.0) {
    printf ("a <= 97.0 -> True: Success.\n");
  } else {
    printf ("a <= 97.0 -> False: Fail.\n");
  }

  if (myChar <= 98.0) {
    printf ("a <= 98.0 -> True: Success.\n");
  } else {
    printf ("a <=98.0 -> False: Fail.\n");
  }

  if (myChar >= 98.0) {
    printf ("a >= 98.0 -> True: Fail.\n");
  } else {
    printf ("a >= 98.0 -> False: Success.\n");
  }

}
