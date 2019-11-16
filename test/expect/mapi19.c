

Character instanceMethod myAdd32 (void) {
  Character new  myChar;

  myChar = ' ';

  if (myChar is Character) {
    return self + myChar;
  } else {
    return self + myChar asCharacter;
  }
}

int main (void) {
  Character new myChar, total;

  myChar = 'C';
  total = myChar myAdd32;
  printf ("%c\n", total);
}
