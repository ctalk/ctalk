
int main () {
  List new l;
  String new item1;
  String new item2;
  String new item3;
  String new item4;

  item1 = "Jeff";
  item2 = "Brad";
  item3 = "Mitch";
  item4 = "Steve";
  
  l push item4;
  l push item1;
  l push item3;
  l push item2;

  l sortAscendingByName;

  l map {
    printf ("%s: %s\n", self name, self);
  }

}
