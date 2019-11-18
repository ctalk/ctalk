
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

  *removed = a1 removeAt "key1";

  val_length1 = strlen (*removed);
  val_length2 = strlen (*removed value);
  printf ("%s, %s, %d = %d, %d = %d\n", *removed, *removed value, 
	  strlen (*removed), val_length1, 
	  strlen (*removed value), val_length2);
  
  val_length1 = strlen ((*removed));
  val_length2 = strlen ((*removed value));
  printf ("%s, %s, %d = %d, %d = %d\n", (*removed), (*removed value), 
	  strlen ((*removed)), val_length1, 
	  strlen ((*removed value)), val_length2);

  val_length1 = strlen (((*removed)));
  val_length2 = strlen (((*removed value)));
  printf ("%s, %s, %d = %d, %d = %d\n", ((*removed)), ((*removed value)), 
	  strlen (((*removed))), val_length1, 
	  strlen (((*removed value))), val_length2);
  
  val_length1 = strlen (*(removed));
  val_length2 = strlen (*(removed value));
  printf ("%s, %s, %d = %d, %d = %d\n", *(removed), *(removed value), 
	  strlen (*(removed)), val_length1, 
	  strlen (*(removed value)), val_length2);
  
  val_length1 = strlen (*((removed)));
  val_length2 = strlen (*((removed value)));
  printf ("%s, %s, %d = %d, %d = %d\n", *((removed)), *((removed value)), 
	  strlen (*((removed))), val_length1, 
	  strlen (*((removed value))), val_length2);
  
  val_length1 = strlen (*(((removed))));
  val_length2 = strlen (*(((removed value))));
  printf ("%s, %s, %d = %d, %d = %d\n", *(((removed))), *(((removed value))), 
	  strlen (*(((removed)))), val_length1, 
	  strlen (*(((removed value)))), val_length2);
  
  printf ("--\n");

  val_length1 = strlen (*removed myMethod);
  val_length2 = strlen (*removed value myMethod);
  printf ("%s, %s, %d = %d, %d = %d\n", *removed myMethod, 
	  *removed value myMethod,
	  strlen (*removed myMethod), val_length1,
	  strlen (*removed value myMethod), val_length2);

  val_length1 = strlen (*(removed) myMethod);
  val_length2 = strlen (*(removed value) myMethod);
  printf ("%s, %s, %d = %d, %d = %d\n", *(removed) myMethod, 
	  *(removed value) myMethod,
	  strlen (*(removed) myMethod), val_length1,
	  strlen (*(removed value) myMethod), val_length2);

  val_length1 = strlen (*((removed)) myMethod);
  val_length2 = strlen (*((removed value)) myMethod);
  printf ("%s, %s, %d = %d, %d = %d\n", *(removed) myMethod, 
	  *((removed value)) myMethod,
	  strlen (*((removed)) myMethod), val_length1,
	  strlen (*((removed value)) myMethod), val_length2);

  val_length1 = strlen (*(((removed))) myMethod);
  val_length2 = strlen (*(((removed value))) myMethod);
  printf ("%s, %s, %d = %d, %d = %d\n", *((removed)) myMethod, 
	  *(((removed value))) myMethod,
	  strlen (*(((removed))) myMethod), val_length1,
	  strlen (*(((removed value))) myMethod), val_length2);


  val_length1 = strlen (*(removed myMethod));
  val_length2 = strlen (*(removed value myMethod));
  printf ("%s, %s, %d = %d, %d = %d\n", *(removed myMethod), 
	  *(removed value myMethod),
	  strlen (*(removed myMethod)), val_length1,
	  strlen (*(removed value myMethod)), val_length2);

  val_length1 = strlen (*((removed myMethod)));
  val_length2 = strlen (*((removed value myMethod)));
  printf ("%s, %s, %d = %d, %d = %d\n", *((removed myMethod)), 
	  *((removed value myMethod)),
	  strlen (*((removed myMethod))), val_length1,
	  strlen (*((removed value myMethod))), val_length2);

  val_length1 = strlen (*(((removed myMethod))));
  val_length2 = strlen (*(((removed value myMethod))));
  printf ("%s, %s, %d = %d, %d = %d\n", *(((removed myMethod))), 
	  *(((removed value myMethod))),
	  strlen (*(((removed myMethod)))), val_length1,
	  strlen (*(((removed value myMethod)))), val_length2);

  val_length1 = strlen ((*removed myMethod));
  val_length2 = strlen ((*removed value myMethod));
  printf ("%s, %s, %d = %d, %d = %d\n", (*removed myMethod), 
	  (*removed value myMethod),
	  strlen ((*removed myMethod)), val_length1,
	  strlen ((*removed value myMethod)), val_length2);

  val_length1 = strlen (((*removed myMethod)));
  val_length2 = strlen (((*removed value myMethod)));
  printf ("%s, %s, %d = %d, %d = %d\n", ((*removed myMethod)), 
	  ((*removed value myMethod)),
	  strlen (((*removed myMethod))), val_length1,
	  strlen (((*removed value myMethod))), val_length2);

  val_length1 = strlen ((((*removed myMethod))));
  val_length2 = strlen ((((*removed value myMethod))));
  printf ("%s, %s, %d = %d, %d = %d\n", (((*removed myMethod))), 
	  (((*removed value myMethod))),
	  strlen ((((*removed myMethod)))), val_length1,
	  strlen ((((*removed value myMethod)))), val_length2);

}
