
int main () {
  AssociativeArray new a;

  a = "key1", "first", "key2", "second", "key3", "third", "key4", "fourth";
  a += "key5", "fifth", "key6", "sixth", "key7", "seventh", "key8", "eighth";

  a mapKeys {
    printf ("%s --> %s\n", self name, *self);
  }
}
