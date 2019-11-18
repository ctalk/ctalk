

Object instanceMethod printObject (void) {
  WriteFileStream new w;
  stdoutStream printOn "name:\t\t\t%s\n", self -> __o_name;
  stdoutStream printOn "classname:\t\t%s\n", self -> __o_classname;
  /* Torture test. */
/*   stdoutStream printOn "class:\t\t\t%s\n", self -> __o_classname asInteger; */
  stdoutStream printOn "class:\t\t\t%p\n", self -> __o_class asSymbol;
  stdoutStream printOn "superclassname:\t\t%s\n", self -> __o_superclassname;
  stdoutStream printOn "superclass:\t\t%p\n", self -> __o_superclass asSymbol;
  stdoutStream printOn "parent:\t\t\t%s\n", self -> __o_p_obj asInteger;
  stdoutStream printOn "value:\t\t\t%s\n", self -> __o_value;
  stdoutStream printOn "instancemethods:\t%p\n", self -> instance_methods;
  stdoutStream printOn "classmethods:\t\t%p\n", self -> class_methods;
  stdoutStream printOn "instancevars:\t\t%p\n", self -> instancevars;
  stdoutStream printOn "classvariables:\t\t%p\n", self -> classvars;
  stdoutStream printOn "scope:\t\t\t%d\n", self -> scope;
  stdoutStream printOn "references:\t\t%d\n", self -> nrefs;
  stdoutStream printOn "next:\t\t\t%p\n", self -> next;
  stdoutStream printOn "previous:\t\t%p\n", self -> prev;
}

int main () {
  Integer new i;
  i printObject;
}
