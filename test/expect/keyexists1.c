
int main () {

  AssociativeArray new a;

  a atPut "key1", "value1";
  a atPut "key2", "value2";
  a atPut "key3", "value3";

  if (a keyExists "key1")
    printf ("key1 exists - Ok\n");
  else
    printf ("key1 does not exist - Not OK\n");
    
  if (a keyExists "key3")
    printf ("key3 exists - Ok\n");
  else
    printf ("key3 does not exist - Not OK\n");
    
  if (a keyExists "key4")
    printf ("key4 exists - Not Ok\n");
  else
    printf ("key4 does not exist - OK\n");
    
}
