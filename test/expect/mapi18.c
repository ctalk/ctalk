

Character instanceMethod myAdd (Character c) {
  if (c is Character) {
    return self + c;
  } else {
    return self + c asCharacter;
  }
}

int main (void) {
  Character new myChar, total;

  myChar = 'C';
  total = myChar myAdd 32;
  printf ("%c\n", total);
}
