
int main () {
  SortedList new l;
  Integer new item1;
  Integer new item2;
  Integer new item3;
  Integer new item4;
  Integer new item5;
  Integer new item6;

  item1 = 1;
  item2 = 2;
  item3 = 3;
  item4 = 4;
  item5 = 5;
  item6 = 6;

  l = item2, item3, item1;

  l map {
    printf ("%d\n", self);
  }

  l = item5, item4, item6;

  l map {
    printf ("%d\n", self);
  }

}
