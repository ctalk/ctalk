
int main () {
  Character new myChar;

  myChar = '0';

  if (myChar || '1') {
    printf ("'0' || '1' -> True: Success.\n");
  } else {
    printf ("'0' || '1': -> False: Fail.\n");
  }

  if (myChar || '\0') {
    printf ("'0' || '\\0' -> True: Success.\n");
  } else {
    printf ("'0' || '\\0': -> False: Fail.\n");
  }

  if (myChar && '1') {
    printf ("'0' && '1' -> True: Success.\n");
  } else {
    printf ("'0' && '1': -> False: Fail.\n");
  }

  if (myChar && '\0') {
    printf ("'0' && '\\0' -> True: Fail.\n");
  } else {
    printf ("'0' && '\\0': -> False: Success.\n");
  }

  if (myChar || 1) {
    printf ("'0' || 1 -> True: Success.\n");
  } else {
    printf ("'0' || 1: -> False: Fail.\n");
  }

  if (myChar || 0) {
    printf ("'0' || 0 -> True: Success.\n");
  } else {
    printf ("'0' || 0: -> False: Fail.\n");
  }

  if (myChar && 1) {
    printf ("'0' && 1 -> True: Success.\n");
  } else {
    printf ("'0' && 1: -> False: Fail.\n");
  }

  if (myChar && 0) {
    printf ("'0' && 0 -> True: Fail.\n");
  } else {
    printf ("'0' && 0: -> False: Success.\n");
  }

  if (myChar || 1l) {
    printf ("'0' || 1l -> True: Success.\n");
  } else {
    printf ("'0' || 1l -> False: Fail.\n");
  }

  if (myChar || 0l) {
    printf ("'0' || 0l -> True: Success.\n");
  } else {
    printf ("'0' || 0l: -> False: Fail.\n");
  }

  if (myChar && 1l) {
    printf ("'0' && 1l -> True: Success.\n");
  } else {
    printf ("'0' && 1l -> False: Fail.\n");
  }

  if (myChar && 0l) {
    printf ("'0' && 0l -> True: Fail.\n");
  } else {
    printf ("'0' && 0l: -> False: Success.\n");
  }

  if (myChar || 1ll) {
    printf ("'0' || 1ll -> True: Success.\n");
  } else {
    printf ("'0' || 1ll -> False: Fail.\n");
  }

  if (myChar || 0ll) {
    printf ("'0' || 0ll -> True: Success.\n");
  } else {
    printf ("'0' || 0ll: -> False: Fail.\n");
  }

  if (myChar && 1ll) {
    printf ("'0' && 1ll -> True: Success.\n");
  } else {
    printf ("'0' && 1ll -> False: Fail.\n");
  }

  if (myChar && 0ll) {
    printf ("'0' && 0ll -> True: Fail.\n");
  } else {
    printf ("'0' && 0ll: -> False: Success.\n");
  }

  if (myChar || 1L) {
    printf ("'0' || 1L -> True: Success.\n");
  } else {
    printf ("'0' || 1L -> False: Fail.\n");
  }

  if (myChar || 0L) {
    printf ("'0' || 0L -> True: Success.\n");
  } else {
    printf ("'0' || 0L: -> False: Fail.\n");
  }

  if (myChar && 1L) {
    printf ("'0' && 1L -> True: Success.\n");
  } else {
    printf ("'0' && 1L -> False: Fail.\n");
  }

  if (myChar && 0L) {
    printf ("'0' && 0L -> True: Fail.\n");
  } else {
    printf ("'0' && 0L: -> False: Success.\n");
  }

  if (myChar || 1.0) {
    printf ("'0' || 1.0 -> True: Success.\n");
  } else {
    printf ("'0' || 1.0 -> False: Fail.\n");
  }

  if (myChar || 0.0) {
    printf ("'0' || 0.0 -> True: Success.\n");
  } else {
    printf ("'0' || 0.0: -> False: Fail.\n");
  }

  if (myChar && 1.0) {
    printf ("'0' && 1.0 -> True: Success.\n");
  } else {
    printf ("'0' && 1.0 -> False: Fail.\n");
  }

  if (myChar && 0.0) {
    printf ("'0' && 0.0 -> True: Fail.\n");
  } else {
    printf ("'0' && 0.0: -> False: Success.\n");
  }

  if (myChar invert) {
    printf ("'0' invert -> True: Fail.\n");
  } else {
    printf ("'0' invert -> False: Success.\n");
  }

}
