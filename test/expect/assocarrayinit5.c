

Integer instanceMethod printAssocArray (void) {
  AssociativeArray new a;
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

  a = "key1", item1, "key2", item2, "key3", item3;

  a mapKeys {
    printf ("%s -> %d\n", self name, *self);
  }

  a = "key4", item4, "key5", item5, "key6", item6;

  a mapKeys {
    printf ("%s --> %d\n", self name, *self);
  }

}

int main () {
  Integer new myInt;
  myInt printAssocArray;
  printf ("--------\n");
  myInt printAssocArray;
}
