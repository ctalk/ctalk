
int main () {
  SortedList new l;
  String new item1;
  String new item2;
  String new item3;
  String new item4;
  
  item1 = "Bill";
  item2 = "Steve";
  item3 = "John";
  item4 = "Jim";

  l pushDescending item1;
  l pushDescending item3;
  printf ("----------------\n");
  l map {
    printf ("%s ", self);
  }
  printf ("\n");
  l pushDescending item2;
  printf ("----------------\n");
  l map {
    printf ("%s ", self);
  }
  printf ("\n");
  l pushDescending item4;
  printf ("----------------\n");
  l map {
    printf ("%s ", self);
  }
  printf ("\n");
}
