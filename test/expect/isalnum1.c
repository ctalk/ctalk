
int main () {
  Character new c;

  c = 'a';
  if (c isAlNum) {
    printf ("a : True -- Pass\n");
  } else {
    printf ("a : False -- Fail\n");
  }
  c = 'Z';
  if (c isAlNum) {
    printf ("Z : True -- Pass\n");
  } else {
    printf ("Z : False -- Fail\n");
  }
  c = '0';
  if (c isAlNum) {
    printf ("0 : True -- Pass\n");
  } else {
    printf ("0 : False -- Fail\n");
  }

  c = '!';
  if (c isAlNum) {
    printf ("! : True -- Fail\n");
  } else {
    printf ("! : False -- Pass\n");
  }

  if ('a' isAlNum) {
    printf ("a : True -- Pass\n");
  } else {
    printf ("a : False -- Fail\n");
  }
  if ('Z' isAlNum) {
    printf ("Z : True -- Pass\n");
  } else {
    printf ("Z : False -- Fail\n");
  }
  if ('0' isAlNum) {
    printf ("0 : True -- Pass\n");
  } else {
    printf ("0 : False -- Fail\n");
  }
  if ('!' isAlNum) {
    printf ("! : True -- Fail\n");
  } else {
    printf ("! : False -- Pass\n");
  }
}
