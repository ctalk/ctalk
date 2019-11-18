
String instanceMethod myMethod (void) {
  return self;
}

int main () {

  AssociativeArray new a1;
  String new value1;
  String new value2;
  String new value3;
  Symbol new removed;

  WriteFileStream classInit;

  value1 = "Value 1";
  value2 = "Value 2";
  value3 = "Value 3";

  a1 atPut "key1", value1;
  a1 atPut "key2", value2;
  a1 atPut "key3", value3;

  *removed = a1 removeAt "key1";

  stdoutStream printOn "%s, %s\n", *removed myMethod, *removed value myMethod;
  stdoutStream printOn "%s, %s\n", (*removed) myMethod, 
    (*removed value) myMethod;
  stdoutStream printOn "%s, %s\n", ((*removed)) myMethod, 
    ((*removed value)) myMethod;
  stdoutStream printOn "%s, %s\n", (((*removed))) myMethod, 
    (((*removed value))) myMethod;
  stdoutStream printOn "%s, %s\n", *(removed) myMethod, 
    *(removed value) myMethod;
  stdoutStream printOn "%s, %s\n", *((removed)) myMethod, 
    *((removed value)) myMethod;
  stdoutStream printOn "%s, %s\n", *(((removed))) myMethod, 
    *(((removed value))) myMethod;
  stdoutStream printOn "%s, %s\n", (*removed myMethod), 
    (*removed value myMethod);
  stdoutStream printOn "%s, %s\n", ((*removed myMethod)), 
    ((*removed value myMethod));
  stdoutStream printOn "%s, %s\n", (((*removed myMethod))), 
    (((*removed value myMethod)));
}
