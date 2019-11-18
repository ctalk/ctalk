Integer instanceMethod myAddr (void) {
  OBJECT *self_alias;
  Symbol new s;
  self_alias = __ctalk_self_internal ();
  /* 
     This *should* mean, "a new Symbol object with the
     address of "self"; i.e., the statement is equivalent
     to 
       *s = self;
     and it is easiest to translate it into that.
  */
  s = &self;
  printf ("The following two addresses should be the same.\n");
  printf ("%p\n", self_alias);
  /*  
   * These semantics may go away. 
   */
  /* printf ("%p\n", (s getValue) addressOf); */
  /* printf ("%p\n", s getValue addressOf); */
  printf ("%p\n", &self);
}

int main () {
  Integer new i;
  i myAddr;
}
