

int main (int argc, char **argv) {

  AssociativeArray new a;
  Key new k;

  a atPut "key1", "value1";
  a atPut "key2", "value2";
  a atPut "key3", "value3";

  k = *a;

  printf ("%s --> %s\n", k name, *k);
}
