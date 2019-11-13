
int main () {
  AssociativeArray new a;

  a init "key1", "first", "key2", "second", "key3", "third", 
    "key4", "fourth";
  a append "key5", "fifth", "key6", "sixth", "key7", "seventh", 
    "key8", "eigth";

  a mapKeys {
    printf ("%s --> %s\n", self name, *self);
  }
}
