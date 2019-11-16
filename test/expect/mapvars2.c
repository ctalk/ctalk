

int main () {
  WriteFileStream new w;
  String new s;
  String new s_element;
  Symbol new varRef;

  s = "";
  w mapClassVariables {
    *varRef = self;
    s_element printOn "%s (%s)\n",
      varRef getValue -> __o_name,
      varRef getValue -> __o_classname;
    s = s + s_element;
    s_element printOn "%d (%s)\n",
      varRef getValue -> __o_value asInteger,
      varRef getValue -> __o_classname;
    s = s + s_element;
  }
  stdoutStream writeFormat "%s\n", s; 
}

