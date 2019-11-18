
String instanceMethod myMethod (void) {
  return self;
}

int main () {

  AssociativeArray new a1;
  String new value1;
  String new value2;
  String new value3;
  Symbol new removed;

  value1 = "Value 1";
  value2 = "Value 2";
  value3 = "Value 3";

  a1 atPut "key1", value1;
  a1 atPut "key2", value2;
  a1 atPut "key3", value3;

  *removed = a1 removeAt "key1";

  printf ("1. %s, %s\n", *removed myMethod, *removed value myMethod);
  printf ("2. %s, %s\n", *(removed) myMethod, *(removed value) myMethod); 
  printf ("3. %s, %s\n", *((removed)) myMethod, *((removed value)) myMethod); 
  printf ("4. %s, %s\n", *(((removed))) myMethod, *(((removed value))) myMethod); 
  printf ("5. %s, %s\n", *(removed myMethod), *(removed value myMethod)); 
  printf ("6. %s, %s\n", *((removed myMethod)), *((removed value myMethod))); 
  printf ("7. %s, %s\n", *(((removed myMethod))), *(((removed value myMethod)))); 

  printf ("8. %s, %s\n", (*removed myMethod), (*removed value myMethod));
  printf ("9. %s, %s\n", ((*removed myMethod)), ((*removed value myMethod)));
  printf ("10. %s, %s\n", (((*removed myMethod))), (((*removed value myMethod))));


}
