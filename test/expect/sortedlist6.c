
int main () {
  SortedList new l;
  Integer new item1;
  Integer new item2;
  Integer new item3;
  Integer new item4;
  
  item1 = 1;
  item2 = 2;
  item3 = 3;
  item4 = 4;

  l pushDescending item1;
  l pushDescending item2;
  printf ("----------------\n");
  l map {
    printf ("%d ", self);
  }
  printf ("\n");
  l pushDescending item4;
  printf ("----------------\n");
  l map {
    printf ("%d ", self);
  }
  printf ("\n");
  l pushDescending item3;
  printf ("----------------\n");
  l map {
    printf ("%d ", self);
  }
  printf ("\n");
}
