

int main () {

  AssociativeArray new a;
  Key new k;

  a atPut "1", "value1";
  a atPut "2", "value2";
  a atPut "3", "value3";
  a atPut "4", "value4";

  k = *a;
  printf ("%s --> %s\n", k name, *k);
  k = k + 2;
  printf ("%s --> %s\n", k name, *k);
}
