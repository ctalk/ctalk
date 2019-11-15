
int main () {

  List new myList;
  Key new k;

  myList push "1";
  myList push "2";
  myList push "3";

  k = *myList;
  while (++k) {
    printf ("%s\n", *k);
  }

  k = *myList;
  printf ("%s\n", *k);
  ++k;
  printf ("%s\n", *k);
}
