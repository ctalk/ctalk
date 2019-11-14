/* If we use the expression, 

   s = s + s_element;

   It seems to cause an untraceable memory leak from the extra
   object that is created.

   Using 

   s += s_element;

   which does not create an extra object, does not cause a leak.
*/

ObjectInspector instanceMethod localFormatClassVariable (Symbol __objectRef) {
  String new s;
  String new s_element;

  returnObjectClass String;

  s_element printOn "class variable:          %s\n", 
    __objectRef getValue -> __o_name;
  s += s_element;

  s_element printOn "class:                   %s (%p)\n",
    __objectRef getValue -> __o_classname asString,
    __objectRef getValue -> __o_class asSymbol;
  s += s_element;

  s_element printOn "superclass:              %s (%p)\n",
    __objectRef getValue -> __o_superclassname,
    __objectRef getValue -> __o_superclass asSymbol;
  s += s_element;

  s_element printOn "value:                   %d (%s)\n",
    /* __objectRef getValue -> instancevars getValue value,
       __objectRef getValue -> instancevars getValue value className; *//***/
    __objectRef getValue value,
    __objectRef getValue value className;
  s += s_element;

  return s;
}

ObjectInspector instanceMethod localFormatInstanceVariable (Symbol __objectRef) {
  String new s;
  String new s_element;

  returnObjectClass String;

  s_element printOn "instance variable:       %s\n", 
    __objectRef getValue -> __o_name;
  /* If we use this, it seems to cause a memory leak... */
    /*  s = s + s_element;*/
  s += s_element;

  s_element printOn "class:                   %s (%p)\n",
    __objectRef getValue -> __o_classname asString,
    __objectRef getValue -> __o_class asSymbol;
  s += s_element;

  s_element printOn "superclass:              %s (%p)\n",
    __objectRef getValue -> __o_superclassname,
    __objectRef getValue -> __o_superclass asSymbol;
  s += s_element;

  s_element printOn "value:                   %s (%s)\n",
    __objectRef getValue -> instancevars getValue value,
    __objectRef getValue -> instancevars getValue value className;
  s += s_element;
  methodReturnObject(s);
}

ObjectInspector instanceMethod localFormatObject (Symbol __objectRef) {
  String new s;
  String new s_element;
  Object new refValue;
  Object new rcvrCopy;
  Symbol new selfVar;
  
  returnObjectClass String;

  s = "";

  s_element printOn "name:       %s\n", __objectRef getValue -> __o_name;
  s += s_element;

  s_element printOn "class:      %s (%p)\n", 
    __objectRef getValue -> __o_classname asString,
    __objectRef getValue -> __o_class asSymbol;
  s += s_element;

  s_element printOn "superclass: %s (%p)\n", 
    __objectRef getValue -> __o_superclassname,
    __objectRef getValue -> __o_superclass asSymbol;
  s += s_element;

  s_element printOn "value:      %s (%s)\n", 
    __objectRef getValue -> instancevars getValue value,
    __objectRef getValue -> __o_classname;
  s += s_element;

  refValue become __objectRef getValue;
  rcvrCopy copy self;
  refValue mapInstanceVariables {
    if (self name != "value") {
      /*
       *  This is the least ambiguous way to write this
       *  loop.
       */
      *selfVar = self;
      s_element = rcvrCopy localFormatInstanceVariable selfVar;
      /*      s = s + s_element; */
      s += s_element;
    }
  }

  refValue mapClassVariables {
    *selfVar = self;
    s_element = rcvrCopy localFormatClassVariable selfVar;
    /*    s = s + s_element; */
    s += s_element;
  }

  methodReturnObject(s);
}

int main () {
  WriteFileStream new w;
  Symbol new s;
  String new str;
  ObjectInspector new inspector;

  *s = w;

  str = inspector localFormatObject s;
  printf ("%s\n", str);
}
