
int main () {

  AssociativeArray new a;
  Symbol new removed;
  String new item1;
  String new item2;
  String new item3;

  item1 = "value1";
  item2 = "value2";
  item3 = "value3";

  a atPut "key1", item1;
  a atPut "key2", item2;
  a atPut "key3", item3;

  *removed = a removeAt "key1";

  a map {
    printf ("%s\n", self value);
  }
  printf ("------------------------\n");
  printf ("%s : %s\n", removed getValue name, removed getValue value);
}
