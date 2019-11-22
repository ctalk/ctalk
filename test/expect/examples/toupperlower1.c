int main () {
  Character new upperCase;
  Character new lowerCase;
  Integer new toggleBit;  

  upperCase = 'Z';
  lowerCase = 'z';
  toggleBit = 100000b;
  
  printf ("%c\n", upperCase ^ toggleBit);
  printf ("%c\n", lowerCase ^ toggleBit);
}
