/*
 *  Constant receivers with subscript terms.
 *
 *  Generates a lot of "Result of Character addition is not an ASCII
 *  character" warnings.
 *  
 */

Integer instanceMethod myMethod (void) {
  char s[20];
  Integer new i;

  xstrcpy (s, "Hello, world!");

     i = (s [0] + s[1]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", (s[0] + s[1]) asInteger); 

     i = (s [0] + 'e') asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", (s[0] + 'e') asInteger); 

     i = ('H' + s[1]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", ('H' + s[1]) asInteger); 

     i = (s[0] + s[1] + s[2]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", (s[0] + s[1] + s[2]) asInteger); 

     i = (s[0] + 'e' + 'l') asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", (s[0] + 'e' + 'l') asInteger); 

     i = ('H' + s[1] + 'l') asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", ('H' + s[1] + 'l') asInteger); 

     i = ('H' + 'e' + s[2]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", ('H' + 'e' + s[2]) asInteger); 

     i = ('H' + s[1] + s[2]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", ('H' + s[1] + s[2]) asInteger); 

     i = (s[0] + 'e' + s[2]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", (s[0] + 'e' + s[2]) asInteger); 

     i = (s[0] + s[1] + 'l') asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", (s[0] + s[1] + 'l') asInteger); 

     i = ((s[0] + s[1]) + s[2]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", ((s[0] + s[1]) + s[2]) asInteger); 

     i = (('H' + s[1]) + s[2]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", (('H' + s[1]) + s[2]) asInteger); 

     i = ((s[0] + 'e') + s[2]) asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", ((s[0] + 'e') + s[2]) asInteger); 

     i = ((s[0] + s[1]) + 'l') asInteger; 
     printf ("%d\n", i); 
     printf ("%d\n", ((s[0] + s[1]) + 'l') asInteger); 

  if (-83 == (s[0] + s[1]) asInteger) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if ((s[0] + s[1]) asInteger == -83) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  if (-1 == (s[0] + s[1]) asInteger) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if ((s[0] + s[1]) asInteger == -1) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }
}

int main () {
  Integer new i;
  i myMethod;
}
