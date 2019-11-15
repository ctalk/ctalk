
List new globalList;

int main () {

  Key new k;
  int i;

  globalList push "1";
  globalList push "2";
  globalList push "3";
  globalList push "4";
  globalList push "5";
  globalList push "6";
  globalList push "7";
  globalList push "8";
  globalList push "9";
  globalList push "10";
  globalList push "11";
  globalList push "12";
  globalList push "13";
  globalList push "14";

  k = *globalList;
  printf ("%s\n", *k);

  i = 2;

  k = k + i;
  printf ("%s\n", *k);

  k = *globalList;
  printf ("%s\n", *k);
  
  k = k + i;
  printf ("%s\n", *k);

  k = *globalList;
  printf ("%s\n", *k);
  
  k = k + i;
  printf ("%s\n", *k);
  k = k + i;
  printf ("%s\n", *k);
  k = k + i;
  printf ("%s\n", *k);
  k = k + i;
  printf ("%s\n", *k);

  printf ("---------------------\n");
  k = *globalList;
  printf ("%s\n", *k);
  
}
