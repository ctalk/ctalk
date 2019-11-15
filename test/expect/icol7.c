

int main (int argc, char **argv) {

  AssociativeArray new a;
  Key new k;
  Key new j;

  a atPut "key1", "value1";
  a atPut "key2", "value2";
  a atPut "key3", "value3";
  a atPut "key4", "value4";

  k = *a;
  j = *a;

  while (++k) {
    printf ("%s --> %s\n", k name, *k);
    j++;
  }

  printf ("-------------------\n");
  printf ("%s\n", j name);
  printf ("-------------------\n");
  while (j--)
    printf ("%s --> %s\n", j name, *j);
    
}
