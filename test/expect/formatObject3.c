ObjectInspector instanceMethod localFormatObject (Symbol __objectRef) {
  String new s;
  String new s_element;
  Object new refValue;
  Symbol new varRef;
  /* Symbol new varValue; *//***/
  Integer new varValue;
  
  returnObjectClass String;

  s = "";

  s_element printOn "name:       %s\n", __objectRef getValue -> __o_name;
  s = s + s_element;

  s_element printOn "class:      %s (%p)\n", 
    __objectRef getValue -> __o_classname asString,
    __objectRef getValue -> __o_class asSymbol;
  s = s + s_element;

  s_element printOn "superclass: %s (%p)\n", 
    __objectRef getValue -> __o_superclassname,
    __objectRef getValue -> __o_superclass asSymbol;
  s = s + s_element;

  s_element printOn "value:      %d (%s)\n", 
    __objectRef getValue -> instancevars getValue value,
    __objectRef getValue -> __o_classname;
  s = s + s_element;

  refValue become __objectRef getValue;
  refValue mapInstanceVariables {
    if (self name != "value") {
      s_element printOn "instance variable:       %s\n", self -> __o_name;
      s = s + s_element;

      s_element printOn "class:                   %s (%p)\n",
 	self -> __o_classname asString,
 	self -> __o_class asSymbol;
      s = s + s_element;

      s_element printOn "superclass:              %s (%p)\n",
 	self -> __o_superclassname,
 	self -> __o_superclass asSymbol;
      s = s + s_element;

      varValue = self -> instancevars value;
      /* s_element printOn "value:                   %d (%s)\n",
  	varValue getValue value,
  	varValue getValue value className; */
      s_element printOn "value:                   %d (%s)\n",
  	varValue, 
  	varValue className;
      s = s + s_element;
    }
  }

  return s;
}

int main () {
  Point new i;
  Symbol new s;
  String new str;
  ObjectInspector new inspector;

  i x = 150;
  i y = 225;

  *s = i;
  str = inspector localFormatObject s;
  printf ("%s\n", str);
}
