
int main () {

  String new s;
  OBJECT *s_alias;
  Integer new attrsInt;
  List new l;
  String new member1;
  String new member2;
  

  s_alias = s;

  attrsInt = s -> attrs;

  printf ("%d == %d == %d\n", s -> attrs, s_alias -> attrs, attrsInt);

  l = member1, member2;

  /* Check for an OBJECT_IS_MEMBER_OF_PARENT_COLLECTION attribute,
     which is set for the list's keys. */
  attrsInt = (*l) -> attrs;

  printf ("%d == %d\n", (*l) -> attrs, attrsInt);
}
