
int main () {
  Symbol new sym;
  Point new p;
  p x = 100;
  p y = 150;

  *sym = p;

  sym getValue mapInstanceVariables {
    if (self -> __o_name != "value") {
      printf ("name:  %s\n", self -> __o_name);
      printf ("value: %d\n", self value -> __o_value asInteger);
    }
  }
}

