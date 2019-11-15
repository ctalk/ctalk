
List new globalList;

int main () {

  Key new k;

  globalList push "1";
  globalList push "2";
  globalList push "3";
  globalList push "4";

  k = *globalList;
  printf ("%s\n", *k);

  k = k + 2;
  printf ("%s\n", *k);

  k = *globalList;
  printf ("%s\n", *k);
  
}
