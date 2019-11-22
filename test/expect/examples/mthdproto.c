int main () {

  Application new app;
  String new src;
  String new prototypes;

  src = app methodSource "Object", "basicNew";

  prototypes = app methodPrototypes src;

  printf ("%s\n", prototypes);

}


