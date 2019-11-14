/* $Id: cvarrcvr1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/* 
 * For a less primitive version of this, see cvarrcvr35.c
 */

/*
 * Also add checks for increment and decrement in cvarrcvr2-3...
 */

int main (int argc, char **argv) {
  Integer new i;
  OBJECT *my_obj;
  my_obj = __ctalkCreateObjectInit ("my_obj", "Integer", "Magnitude", 
				    LOCAL_VAR, "255");
  /* __objRefCntSet (&my_obj, 1); */
  my_obj incReferenceCount;
  i = my_obj value;
  printf ("%d\n", i);
  printf ("%d\n", my_obj value);
  /* __ctalkDeleteObject (my_obj); */
  my_obj delete;
}

