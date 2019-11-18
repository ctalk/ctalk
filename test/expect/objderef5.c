

Object instanceMethod printObject (Object i) {
  WriteFileStream new w;
  Integer new mbrInteger;
  String new mbrString;
  Symbol new mbrSymbol;

  mbrString = i -> __o_name;
  printf ("name:\t\t\t%s\n", mbrString);
  mbrString = i -> __o_classname;
  printf ("classname:\t\t%s\n", mbrString);
  mbrSymbol = i -> __o_class;
  printf ("class:\t\t\t%p\n", mbrSymbol);
  mbrString = i -> __o_superclassname;
  printf ("superclassname:\t\t%s\n", mbrString);
  mbrSymbol = i -> __o_superclass;
  printf ("superclass:\t\t%p\n", mbrSymbol);
  mbrSymbol = i -> __o_p_obj;
  printf ("parent:\t\t\t%p\n", mbrSymbol);
  mbrString = i -> __o_value;
  printf ("value:\t\t\t%s\n", mbrString);
  mbrInteger = i -> scope;
  printf ("scope:\t\t\t%d\n", mbrInteger);
  mbrInteger = i -> nrefs;
  printf ("references:\t\t%d\n", mbrInteger);
  mbrSymbol = i -> instancevars;
  printf ("instancevars:\t\t%p\n", mbrSymbol);
  mbrSymbol = i -> classvars;
  printf ("classvars:\t\t%p\n", mbrSymbol);
  mbrInteger = i -> instance_methods;
  printf ("instance methods:\t%p\n", mbrInteger);
  mbrInteger = i -> class_methods;
  printf ("class methods:\t\t%p\n", mbrInteger);
  mbrSymbol = i -> prev;
  printf ("prev:\t\t\t%p\n", mbrSymbol);
  mbrSymbol = i -> next;
  printf ("next:\t\t\t%p\n", mbrSymbol);
}

int main () {
  Integer new i;
  i printObject i;
}
