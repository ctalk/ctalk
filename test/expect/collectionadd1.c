

void printElement (char *s) {

  printf ("%s\n", s);
}

int main () {

  List new myList;

  myList push "1";
  myList push "2";
  myList push "3";

  printElement (*(myList + 1));

}
