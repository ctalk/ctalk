
/* Like assocdel1.c, except with constant parameters. */
int main () {

  AssociativeArray new a;
  Symbol new removed;

  a atPut "key1", "value1";
  a atPut "key2", "value2";
  a atPut "key3", "value3";

  *removed = a removeAt "key1";

  a map {
    printf ("%s\n", self value);
  }
  printf ("------------------------\n");
  printf ("%s : %s\n", removed getValue name, removed getValue value);
}
