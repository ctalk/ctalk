
Object instanceMethod localDeref (char *mbr) {
  String new s;
  OBJECT *result, *symbol_result;
  char buf[MAXMSG];
  int objval;

  returnObjectClass Any;

  sprintf (buf, "self -> %s", mbr);

  result = __ctalkEvalExpr (buf);

  if (result) {
    __ctalkRegisterUserObject (result);
    if (result -> attrs & OBJECT_VALUE_IS_BIN_INT) {
      objval = *(int *)result -> __o_value;
    } else {
      objval = atoi (result -> instancevars -> __o_value);
    }
    sprintf (buf, "%#x", objval);
    if (obj_ref_str(buf)) {
      symbol_result = __ctalkCreateObjectInit ("result", 
					"Symbol", "Object",
					LOCAL_VAR, buf);
      __ctalkRegisterUserObject (symbol_result);
      return symbol_result;
    } else {
      return result;
    }
  } else {
    return NULL;
  }
}

int main () {
  Integer new i;
  Integer new mbrInteger;
  String new mbrString;
  Symbol new mbrSymbol;

  mbrString = i localDeref __o_name;
  printf ("name:\t\t\t%s\n", mbrString);
  mbrString = i localDeref __o_classname;
  printf ("classname:\t\t%s\n", mbrString);
  mbrSymbol = i localDeref __o_class;
  printf ("class:\t\t\t%p\n", mbrSymbol);
  mbrString = i localDeref __o_superclassname;
  printf ("superclassname:\t\t%s\n", mbrString);
  mbrSymbol = i localDeref __o_superclass;
  printf ("superclass:\t\t%p\n", mbrSymbol);
  mbrSymbol = i localDeref __o_p_obj;
  printf ("parent:\t\t\t%p\n", mbrSymbol);
  mbrString = i localDeref __o_value;
  printf ("value:\t\t\t%s\n", mbrString);
  mbrInteger = i localDeref scope;
  printf ("scope:\t\t\t%d\n", mbrInteger);
  mbrInteger = i localDeref nrefs;
  printf ("references:\t\t%d\n", mbrInteger);
  mbrSymbol = i localDeref instancevars;
  printf ("instancevars:\t\t%p\n", mbrSymbol);
  mbrSymbol = i localDeref classvars;
  printf ("classvars:\t\t%p\n", mbrSymbol);
  mbrInteger = i localDeref instance_methods;
  printf ("instance methods:\t%p\n", mbrInteger);
  mbrInteger = i localDeref class_methods;
  printf ("class methods:\t\t%p\n", mbrInteger);
  mbrSymbol = i localDeref prev;
  printf ("prev:\t\t\t%p\n", mbrSymbol);
  mbrSymbol = i localDeref next;
  printf ("next:\t\t\t%p\n", mbrSymbol);

}
