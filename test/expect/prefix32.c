
String instanceMethod myMethod (void) {
  return self;
}

int main () {

  AssociativeArray new a1;
  String new value1;
  String new value2;
  String new value3;
  Symbol new removed;
  int val_length1, val_length2;

  value1 = "Value 1";
  value2 = "Value 2";
  value3 = "Value 3";

  a1 atPut "key1", value1;
  a1 atPut "key2", value2;
  a1 atPut "key3", value3;

  a1 mapKeys {

    val_length1 = strlen (*self myMethod);
    val_length2 = strlen (*self value myMethod);
    printf ("%s, %s, %d = %d, %d = %d\n", *self, *self value, 
	    strlen (*self), val_length1, 
	    strlen (*self value), val_length2);
  
    val_length1 = strlen ((*self myMethod));
    val_length2 = strlen ((*self value myMethod));
    printf ("%s, %s, %d = %d, %d = %d\n", (*self myMethod), 
	    (*self value myMethod), 
	    strlen ((*self myMethod)), val_length1, 
	    strlen ((*self value myMethod)), val_length2);
  
    val_length1 = strlen (((*self myMethod)));
    val_length2 = strlen (((*self value myMethod)));
    printf ("%s, %s, %d = %d, %d = %d\n", *self, *self value, 
	    strlen (((*self))), val_length1, 
	    strlen (((*self value))), val_length2);
  
    val_length1 = strlen ((((*self myMethod))));
    val_length2 = strlen ((((*self value myMethod))));
    printf ("%s, %s, %d = %d, %d = %d\n", *self, *self value, 
	    strlen ((((*self myMethod)))), val_length1, 
	    strlen ((((*self value myMethod)))), val_length2);
  
    val_length1 = strlen (*(self) myMethod);
    val_length2 = strlen (*(self value) myMethod);
    printf ("%s, %s, %d = %d, %d = %d\n", *self, *self value, 
	    strlen (*(self) myMethod), val_length1, 
	    strlen (*(self value) myMethod), val_length2);
  
    val_length1 = strlen (*((self)) myMethod);
    val_length2 = strlen (*((self value)) myMethod);
    printf ("%s, %s, %d = %d, %d = %d\n", *self, *self value, 
	    strlen (*((self)) myMethod), val_length1, 
	    strlen (*((self value)) myMethod), val_length2);
  
    val_length1 = strlen (*(((self))) myMethod);
    val_length2 = strlen (*(((self value))) myMethod);
    printf ("%s, %s, %d = %d, %d = %d\n", *self, *self value, 
	    strlen (*(((self))) myMethod), val_length1, 
	    strlen (*(((self value))) myMethod), val_length2);

    printf ("--\n");
  }

}
