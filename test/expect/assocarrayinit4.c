
int main () {
  AssociativeArray new a;

  a = "key1", "item1", "key2", "item2", "key3", "item3";

  a mapKeys {
    printf ("%s --> %s\n", self name, *self);
  }

  a = "key4", "item4", "key5", "item5", "key6", "item6";

  a mapKeys {
    printf ("%s --> %s\n", self name, *self);
  }

}
