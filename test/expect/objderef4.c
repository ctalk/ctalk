

Object instanceMethod printObject (Object obj) {
  WriteFileStream new w;
  stdoutStream printOn "name:\t\t\t%s\n", obj -> __o_name;
  stdoutStream printOn "classname:\t\t%s\n", obj -> __o_classname;
  /* Torture test. */
/*   stdoutStream printOn "class:\t\t\t%s\n", obj -> __o_classname asInteger; */
  stdoutStream printOn "class:\t\t\t%p\n", obj -> __o_class asSymbol;
  stdoutStream printOn "superclassname:\t\t%s\n", obj -> __o_superclassname;
  stdoutStream printOn "superclass:\t\t%p\n", obj -> __o_superclass asSymbol;
  stdoutStream printOn "parent:\t\t\t%s\n", obj -> __o_p_obj asInteger;
  stdoutStream printOn "value:\t\t\t%s\n", obj -> __o_value;
  stdoutStream printOn "instancemethods:\t%p\n", obj -> instance_methods;
  stdoutStream printOn "classmethods:\t\t%p\n", obj -> class_methods;
  stdoutStream printOn "instancevars:\t\t%p\n", obj -> instancevars;
  stdoutStream printOn "classvariables:\t\t%p\n", obj -> classvars;
  stdoutStream printOn "scope:\t\t\t%d\n", obj -> scope;
  stdoutStream printOn "references:\t\t%d\n", obj -> nrefs;
  stdoutStream printOn "next:\t\t\t%p\n", obj -> next;
  stdoutStream printOn "previous:\t\t%p\n", obj -> prev;
}

int main () {
  Integer new i;
  i printObject i;
}
