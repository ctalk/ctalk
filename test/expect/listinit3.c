
int main () {
  List new l;
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

  l = item1, item2, item3;

  l map {
    printf ("%d\n", self);
  }

  l = item4, item5, item6;

  l map {
    printf ("%d\n", self);
  }

}
