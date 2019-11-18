
int main () {
  WriteFileStream new w;
  Integer new i;

  stdoutStream printOn "name:\t\t\t%s\n", i -> __o_name;
  stdoutStream printOn "classname:\t\t%s\n", i -> __o_classname;
  /* Torture test. */
/*   stdoutStream printOn "class:\t\t\t%s\n", i -> __o_classname asInteger; */
  stdoutStream printOn "class:\t\t\t%p\n", i -> __o_class asSymbol;
  stdoutStream printOn "superclassname:\t\t%s\n", i -> __o_superclassname;
  stdoutStream printOn "superclass:\t\t%p\n", i -> __o_superclass asSymbol;
  stdoutStream printOn "parent:\t\t\t%s\n", i -> __o_p_obj asInteger;
  stdoutStream printOn "value:\t\t\t%s\n", i -> __o_value;
  stdoutStream printOn "instancemethods:\t%p\n", i -> instance_methods;
  stdoutStream printOn "classmethods:\t\t%p\n", i -> class_methods;
  stdoutStream printOn "instancevars:\t\t%p\n", i -> instancevars;
  stdoutStream printOn "classvariables:\t\t%p\n", i -> classvars;
  stdoutStream printOn "scope:\t\t\t%d\n", i -> scope;
  stdoutStream printOn "references:\t\t%d\n", i -> nrefs;
  stdoutStream printOn "next:\t\t\t%p\n", i -> next;
  stdoutStream printOn "previous:\t\t%p\n", i -> prev;
}
