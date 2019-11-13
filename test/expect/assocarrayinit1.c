
int main () {
  AssociativeArray new a;

  a init key1, "value1", key2, "value2", key3, "value3";

  a mapKeys {
    printf ("%s --> %s\n", self name, *self);
  }

}
